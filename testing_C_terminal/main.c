//
//  main.c
//  testing_C_terminal
//
//  Created by Simon Anderson on 13/09/23.
//

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "obj_to_ccm.h"
#include "fastOBJ/fast_obj.h"

#ifndef CC_IMPLEMENTATION
#  include "CatmullClark.h"
#endif

#ifndef CBF_IMPLEMENTATION
#  include "ConcurrentBitField.h"
#endif

#ifndef CC_LOG
#    include <stdio.h>
#    define CC_LOG(format, ...) do { fprintf(stdout, format "\n", ##__VA_ARGS__); fflush(stdout); } while(0)
#endif

#ifndef CC_ASSERT
#    include <assert.h>
#    define CC_ASSERT(x) assert(x)
#endif

#ifndef CC_MALLOC
#    include <stdlib.h>
#    define CC_MALLOC(x) (malloc(x))
#    define CC_FREE(x) (free(x))
#else
#    ifndef CC_FREE
#        error CC_MALLOC defined without CC_FREE
#    endif
#endif


/**
 * Print the current working directory of the code being executed.
 */
void getCurrentWorkingDirectory(void) {
    char cwd[1024];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}

/**
 WIP
 Converts the fastObjMesh into a cc_Mesh
 
 - Trying to leverage the speed of fast Obj.
 */
cc_Mesh *convertFastMesh(const fastObjMesh *fastMesh) {
    cc_Mesh *mesh;
    
    // Used to store the start and end time of a method, to calculate the duration.
    clock_t start_time, end_time, start_func_time, end_func_time;
    
    mesh = (cc_Mesh *)CC_MALLOC(sizeof(*mesh));
    
    CC_LOG("Face     Count: %i", fastMesh->face_count);
    // The following have a dummy entry at index 0
    CC_LOG("Position Count: %i", fastMesh->position_count -1);
    CC_LOG("UV       Count: %i", fastMesh->texcoord_count -1);
    CC_LOG("Normal   Count: %i", fastMesh->normal_count -1);
    
    mesh->faceCount   = fastMesh->face_count;
    mesh->vertexCount = fastMesh->position_count - 1;
    mesh->uvCount     = fastMesh->texcoord_count - 1;
    
    start_func_time = clock();
    
    // The Sum is equal to the total amount of edges for each face.
    // eg. 2 Faces, each with 4 edges, total is 8 halfedges.
    int32_t halfEdgeCount = 0;
    
    for (int i=0;i<fastMesh->face_count;i++) {
        
        for (int ii=0; ii<fastMesh->face_vertices[i]; ii++) {
                        
            halfEdgeCount +=1;
        }
    }
    
    CC_LOG("Half Edge Count: %i", halfEdgeCount);
    mesh->halfedgeCount = halfEdgeCount;
    
    
    // Populated
    // - faceCount
    // - vertCount
    // - uvCount
    // - halfedgeCount
    
    mesh->halfedges = (cc_Halfedge *)CC_MALLOC(sizeof(cc_Halfedge) * mesh->halfedgeCount);
    mesh->vertexPoints = (cc_VertexPoint *)CC_MALLOC(sizeof(cc_VertexPoint) * mesh->vertexCount);
    mesh->uvs = (cc_VertexUv *)CC_MALLOC(sizeof(cc_VertexUv) * mesh->uvCount);
    
    
    // -- ObjLoadMeshData function --
    
    cbf_BitField *faceIterator = cbf_Create(mesh->halfedgeCount + 1);
    
    // Used to access the array data
    cc_VertexPoint *vertexPoints = mesh->vertexPoints;
    cc_VertexUv *uvs = mesh->uvs;
    cc_Halfedge *halfedges = mesh->halfedges;
    
    // reset for counting again
    halfEdgeCount = 0;
    
    unsigned int index = 0;
    
    cbf_SetBit(faceIterator, 0, 1u);
    
    for (int i=0;i<fastMesh->face_count;i++) {
        
        unsigned int verticesPerFace = fastMesh->face_vertices[i];
        
//        CC_LOG("  Face index: %i",i);
        
        for (int ii=0; ii<verticesPerFace; ii++) {
            
            fastObjIndex faceObjIdx = fastMesh->indices[index];
            
            // Index position of the faces Vertex, Normal, Tangent per a Face Vertex.
            fastObjUInt vertIndex    = faceObjIdx.p;
            fastObjUInt normalIndex  = faceObjIdx.n;
            fastObjUInt tangentIndex = faceObjIdx.t;
            
            fastObjUInt vPos = vertIndex * 3;
            fastObjUInt tPos = tangentIndex * 2;
            fastObjUInt nPos = normalIndex * 3;
            

            // Assign the above data to the mesh
            (*vertexPoints).array[0] = fastMesh->positions[vPos];
            (*vertexPoints).array[1] = fastMesh->positions[vPos+1];
            (*vertexPoints).array[2] = fastMesh->positions[vPos+2];
            vertexPoints++;
            
            // Only evaluate UV's if they exist, fastObj's first entry is a placeholder
            if ( fastMesh->texcoord_count > 1 ) {
                (*uvs).array[0] = fastMesh->texcoords[tPos];
                (*uvs).array[1] = fastMesh->texcoords[tPos+1];
                uvs++;
            }
            
            (*halfedges).twinID = -1;
            (*halfedges).edgeID = -1;
            (*halfedges).vertexID = vertIndex - 1;
            (*halfedges).uvID = tangentIndex - 1;

//            CC_LOG("HE %i %i %i %i",
//                   (*halfedges).vertexID,
//                   (*halfedges).uvID,
//                   (*halfedges).edgeID,
//                   (*halfedges).twinID
//                   );

            halfedges++;
            
            // Debugging purposes, All data looks to be correct.
//            cc_VertexPoint vertPoint = {{fastMesh->positions[vPos], fastMesh->positions[vPos+1], fastMesh->positions[vPos+2]}};
//            cc_VertexUv vertUV = {{fastMesh->texcoords[tPos], fastMesh->texcoords[tPos+1]}};
//            cc_VertexPoint vertNormal = {{fastMesh->normals[nPos], fastMesh->normals[nPos+1], fastMesh->normals[nPos+2]}};
//            CC_LOG("    Face Count: %i, %i, %i, %i, [%f, %f, %f] , [%f, %f, %f] , [%f, %f]", verticesPerFace,
//                   vertIndex, normalIndex, tangentIndex,
//                   vertPoint.x,  vertPoint.y,  vertPoint.z,
//                   vertNormal.x, vertNormal.y, vertNormal.z,
//                   vertUV.u, vertUV.v);
            
            index+=1;
            
            halfEdgeCount +=1;
        }
        cbf_SetBit(faceIterator, halfEdgeCount, 1u);
    }
    
    
    cbf_Reduce(faceIterator);
    LoadFaceMappings(mesh, faceIterator);
    
    CC_LOG("Half Edges");
    for (int i =0; i<mesh->halfedgeCount; i++) {
        CC_LOG("id:%i  vertID:%i  uvID:%i", i,
               mesh->halfedges[i].vertexID,
               mesh->halfedges[i].uvID);
    }
    
    end_func_time = clock();
    CC_LOG("Complete (%f's) - Load mesh", (double)(end_func_time - start_func_time) / CLOCKS_PER_SEC);
    
    
    start_func_time = clock();
    ComputeTwins(mesh);
    end_func_time = clock();
    CC_LOG("Complete (%f's) - Compute Twin", (double)(end_func_time - start_func_time) / CLOCKS_PER_SEC);

    
    start_func_time = clock();
    LoadEdgeMappings(mesh);
    end_func_time = clock();
    CC_LOG("Complete (%f's) - Edge Mapping", (double)(end_func_time - start_func_time) / CLOCKS_PER_SEC);
    
    
    start_func_time = clock();
    LoadVertexHalfedges(mesh);
    end_func_time = clock();
    CC_LOG("Complete (%f's) - Vertex Halfedges", (double)(end_func_time - start_func_time) / CLOCKS_PER_SEC);
    
    // TODO: ObjLoadCreaseData - obj_to_ccm #764
    // TODO: MakeBoundaries Sharp - obj_to_ccm #772
    // TODO: Compute Crease Neighbors - obj_to_ccm #781
    
    return mesh;
}




int main(int argc, const char * argv[]) {
    clock_t start_time, end_time;
    
    
    const char *filename = "./CubeTest.obj";
    // Commented out as this seems to fail
//     const char *filename = "./Dude.obj";
//         const char *filename = "./StanfordBunny.obj";
    
    CC_LOG("Loading: %s", filename);
    
    // Import the obj data using fastOBJ
    start_time = clock();
    fastObjMesh *fastMesh = fast_obj_read(filename);
    end_time = clock();
    CC_LOG("fastOBJ Load: %f's", (double)(end_time-start_time)/CLOCKS_PER_SEC);
    
//    convertFastMesh(fastMesh);
    cc_Mesh *mesh = convertFastMesh(fastMesh);
    
    // Loading the Mesh also performs the ConcurrentBitField process.
//    cc_Mesh *mesh = LoadObj(filename);
    
    getCurrentWorkingDirectory();
    
    CC_LOG("LOADED OBJECT ");
    CC_LOG("  Edge Count:     %d", mesh->edgeCount);
    CC_LOG("  HalfEdge Count: %d", mesh->halfedgeCount);
    CC_LOG("  Face Count:     %d", mesh->faceCount);
    CC_LOG("  UV Count Count: %d", mesh->uvCount);
    CC_LOG("  Vertex Count:   %d", mesh->vertexCount);
    
    CC_LOG("  V: %i", ccm_VertexCount(mesh));
    CC_LOG("  U: %i", ccm_UvCount(mesh));
    CC_LOG("  H: %i", ccm_HalfedgeCount(mesh));
    CC_LOG("  C: %i", ccm_CreaseCount(mesh));
    CC_LOG("  E: %i", ccm_EdgeCount(mesh));
    CC_LOG("  F: %i", ccm_FaceCount(mesh));
    
    // Performing Sub Division test
    
    // Smoothing Depth
    int depth = 1;
    
    start_time = clock();
    cc_Subd *subd = ccs_Create(mesh, depth);
    ccs_RefineCreases(subd);
    ccs_RefineHalfedges(subd);
    ccs_RefineVertexPoints_Scatter(subd);
    ccs_RefineVertexUvs(subd);
    end_time = clock();
    CC_LOG("Subdivision time, 1 subdiv: %f", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    CC_LOG("  Verts: %i", ccm_VertexCountAtDepth(subd->cage, 1));
    CC_LOG("  Edges: %i", ccm_EdgeCountAtDepth(subd->cage, 1));
    CC_LOG("  Faces: %i", ccm_FaceCountAtDepth(subd->cage,1));
    
    
    for (int i =0; i<ccm_VertexCountAtDepth(subd->cage, depth); i++) {
        const float *v = ccs_VertexPoint(subd, i, depth).array;
        CC_LOG("     %f %f %f", v[0], v[1], v[2] );
    }
    
    // CLEAN UP
    ccm_Release(mesh);
    fast_obj_destroy(fastMesh);
    return 0;
}
