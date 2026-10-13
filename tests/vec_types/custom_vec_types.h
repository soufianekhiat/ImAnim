// Strict custom vector types for testing IAM_VEC2/IAM_VEC4: no .x/.y members and no
// implicit conversions, so any missed conversion in im_anim.cpp fails to compile.
// Force-include it in every translation unit (-include / /FI).
#pragma once
struct TestVec2 { float a, b; };
struct TestVec4 { float a, b, c, d; };
#define IAM_VEC2              TestVec2
#define IAM_VEC2_TO_IM(v)     ImVec2((v).a, (v).b)
#define IAM_VEC2_FROM_IM(v)   TestVec2{ (v).x, (v).y }
#define IAM_VEC4              TestVec4
#define IAM_VEC4_TO_IM(v)     ImVec4((v).a, (v).b, (v).c, (v).d)
#define IAM_VEC4_FROM_IM(v)   TestVec4{ (v).x, (v).y, (v).z, (v).w }
