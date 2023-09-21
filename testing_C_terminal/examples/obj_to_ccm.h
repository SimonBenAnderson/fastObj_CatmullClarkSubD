#pragma once

#ifndef CC_IMPLEMENTATION
#  include "CatmullClark.h"
#endif

#ifndef CBF_IMPLEMENTATION
#  include "ConcurrentBitField.h"
#endif

void LoadFaceMappings(cc_Mesh *mesh, const cbf_BitField *faceIterator);
void ComputeTwins(cc_Mesh *mesh);
void LoadEdgeMappings(cc_Mesh *mesh);
void LoadVertexHalfedges(cc_Mesh *mesh);

CCDEF cc_Mesh *LoadObj(const char *filename);

