#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <immintrin.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#ifndef __LUABASE_RUNTIME__
#define __LUABASE_RUNTIME__
char _lb_buf[512];
char _lb_buf2[512];
static inline char* _lb_s(char* x)        { return x; }
static inline char* _lb_cs(const char* x) { return (char*)x; }
static inline char* _lb_f(float x)        { snprintf(_lb_buf,sizeof(_lb_buf),"%g",x); return _lb_buf; }
static inline char* _lb_d(double x)       { snprintf(_lb_buf,sizeof(_lb_buf),"%.10g",x); return _lb_buf; }
static inline char* _lb_i(int x)          { snprintf(_lb_buf,sizeof(_lb_buf),"%d",x); return _lb_buf; }
static inline char* _lb_l(long x)         { snprintf(_lb_buf,sizeof(_lb_buf),"%ld",x); return _lb_buf; }
static inline char* _lb_u(short x)        { snprintf(_lb_buf,sizeof(_lb_buf),"%d",(int)x); return _lb_buf; }
static inline char* _lb_b(int x)          { return x ? "true" : "false"; }
static inline char* _lb_c(char x)         { _lb_buf[0]=x; _lb_buf[1]='\0'; return _lb_buf; }
static inline char* _lb_m(__m256 v)  { float f[8]; _mm256_storeu_ps(f,v); snprintf(_lb_buf,sizeof(_lb_buf),"[%g,%g,%g,%g,%g,%g,%g,%g]",f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7]); return _lb_buf; }
static inline char* _lb_mi(__m256i v) { union{__m256i v;int i[8];}u; u.v=v; snprintf(_lb_buf,sizeof(_lb_buf),"[%d,%d,%d,%d,%d,%d,%d,%d]",u.i[0],u.i[1],u.i[2],u.i[3],u.i[4],u.i[5],u.i[6],u.i[7]); return _lb_buf; }
#define TO_STR(x) _Generic((x),char*:_lb_s,const char*:_lb_cs,__m256:_lb_m,__m256i:_lb_mi,float:_lb_f,double:_lb_d,int:_lb_i,long:_lb_l,short:_lb_u,bool:_lb_b,char:_lb_c,default:_lb_i)(x)
static jmp_buf* _lb_exc_active = NULL;
static char*    _lb_exc_msg    = NULL;
static inline void _lb_throw(const char* msg) {
    if(_lb_exc_active) {
        if(_lb_exc_msg) snprintf(_lb_exc_msg,512,"%s",msg);
        longjmp(*_lb_exc_active,1);
    } else {
        fprintf(stderr,"[throw] unhandled: %s\n",msg); exit(1);
    }
}
#endif /* __LUABASE_RUNTIME__ */

#line 1 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    float x;
    float y;
} vec2f;

#line 5 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    float x;
    float y;
    float z;
} vec3f;

#line 10 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    float x;
    float y;
    float z;
    float w;
} vec4f;

#line 16 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    vec4f x;
    vec4f y;
    vec4f z;
    vec4f w;
} mat4f;

#line 22 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    float x;
    float y;
    float z;
    float w;
} quatf;

#line 28 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    vec3f origin;
    vec3f direction;
} rayf;

#line 32 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    vec3f center;
    float radius;
} spheref;

#line 36 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    vec3f min;
    vec3f max;
} aabbf;

#line 40 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    vec3f center;
    float size;
} cubef;

#line 44 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    vec3f normal;
    float distance;
} plane3f;

#line 48 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    float data[16];
} mat4fa;

#line 53 "/usr/include/lb/MathGraphic.lh"
typedef struct {
    bool hit;
    float t;
} RayIntersectResult;

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <immintrin.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#ifndef __LUABASE_RUNTIME__
#define __LUABASE_RUNTIME__
char _lb_buf[512];
char _lb_buf2[512];
static inline char* _lb_s(char* x)        { return x; }
static inline char* _lb_cs(const char* x) { return (char*)x; }
static inline char* _lb_f(float x)        { snprintf(_lb_buf,sizeof(_lb_buf),"%g",x); return _lb_buf; }
static inline char* _lb_d(double x)       { snprintf(_lb_buf,sizeof(_lb_buf),"%.10g",x); return _lb_buf; }
static inline char* _lb_i(int x)          { snprintf(_lb_buf,sizeof(_lb_buf),"%d",x); return _lb_buf; }
static inline char* _lb_l(long x)         { snprintf(_lb_buf,sizeof(_lb_buf),"%ld",x); return _lb_buf; }
static inline char* _lb_u(short x)        { snprintf(_lb_buf,sizeof(_lb_buf),"%d",(int)x); return _lb_buf; }
static inline char* _lb_b(int x)          { return x ? "true" : "false"; }
static inline char* _lb_c(char x)         { _lb_buf[0]=x; _lb_buf[1]='\0'; return _lb_buf; }
static inline char* _lb_m(__m256 v)  { float f[8]; _mm256_storeu_ps(f,v); snprintf(_lb_buf,sizeof(_lb_buf),"[%g,%g,%g,%g,%g,%g,%g,%g]",f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7]); return _lb_buf; }
static inline char* _lb_mi(__m256i v) { union{__m256i v;int i[8];}u; u.v=v; snprintf(_lb_buf,sizeof(_lb_buf),"[%d,%d,%d,%d,%d,%d,%d,%d]",u.i[0],u.i[1],u.i[2],u.i[3],u.i[4],u.i[5],u.i[6],u.i[7]); return _lb_buf; }
#define TO_STR(x) _Generic((x),char*:_lb_s,const char*:_lb_cs,__m256:_lb_m,__m256i:_lb_mi,float:_lb_f,double:_lb_d,int:_lb_i,long:_lb_l,short:_lb_u,bool:_lb_b,char:_lb_c,default:_lb_i)(x)
static jmp_buf* _lb_exc_active = NULL;
static char*    _lb_exc_msg    = NULL;
static inline void _lb_throw(const char* msg) {
    if(_lb_exc_active) {
        if(_lb_exc_msg) snprintf(_lb_exc_msg,512,"%s",msg);
        longjmp(*_lb_exc_active,1);
    } else {
        fprintf(stderr,"[throw] unhandled: %s\n",msg); exit(1);
    }
}
#endif /* __LUABASE_RUNTIME__ */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <immintrin.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#ifndef __LUABASE_RUNTIME__
#define __LUABASE_RUNTIME__
char _lb_buf[512];
char _lb_buf2[512];
static inline char* _lb_s(char* x)        { return x; }
static inline char* _lb_cs(const char* x) { return (char*)x; }
static inline char* _lb_f(float x)        { snprintf(_lb_buf,sizeof(_lb_buf),"%g",x); return _lb_buf; }
static inline char* _lb_d(double x)       { snprintf(_lb_buf,sizeof(_lb_buf),"%.10g",x); return _lb_buf; }
static inline char* _lb_i(int x)          { snprintf(_lb_buf,sizeof(_lb_buf),"%d",x); return _lb_buf; }
static inline char* _lb_l(long x)         { snprintf(_lb_buf,sizeof(_lb_buf),"%ld",x); return _lb_buf; }
static inline char* _lb_u(short x)        { snprintf(_lb_buf,sizeof(_lb_buf),"%d",(int)x); return _lb_buf; }
static inline char* _lb_b(int x)          { return x ? "true" : "false"; }
static inline char* _lb_c(char x)         { _lb_buf[0]=x; _lb_buf[1]='\0'; return _lb_buf; }
static inline char* _lb_m(__m256 v)  { float f[8]; _mm256_storeu_ps(f,v); snprintf(_lb_buf,sizeof(_lb_buf),"[%g,%g,%g,%g,%g,%g,%g,%g]",f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7]); return _lb_buf; }
static inline char* _lb_mi(__m256i v) { union{__m256i v;int i[8];}u; u.v=v; snprintf(_lb_buf,sizeof(_lb_buf),"[%d,%d,%d,%d,%d,%d,%d,%d]",u.i[0],u.i[1],u.i[2],u.i[3],u.i[4],u.i[5],u.i[6],u.i[7]); return _lb_buf; }
#define TO_STR(x) _Generic((x),char*:_lb_s,const char*:_lb_cs,__m256:_lb_m,__m256i:_lb_mi,float:_lb_f,double:_lb_d,int:_lb_i,long:_lb_l,short:_lb_u,bool:_lb_b,char:_lb_c,default:_lb_i)(x)
static jmp_buf* _lb_exc_active = NULL;
static char*    _lb_exc_msg    = NULL;
static inline void _lb_throw(const char* msg) {
    if(_lb_exc_active) {
        if(_lb_exc_msg) snprintf(_lb_exc_msg,512,"%s",msg);
        longjmp(*_lb_exc_active,1);
    } else {
        fprintf(stderr,"[throw] unhandled: %s\n",msg); exit(1);
    }
}
#endif /* __LUABASE_RUNTIME__ */

#line 62 "/usr/include/lb/MathGraphic.lh"
float clamp(float x, float minVal, float maxVal) {
#line 63 "/usr/include/lb/MathGraphic.lh"
    if (((x<minVal))) {
#line 64 "/usr/include/lb/MathGraphic.lh"
    return minVal;
} else {
#line 66 "/usr/include/lb/MathGraphic.lh"
    if (((x>maxVal))) {
#line 67 "/usr/include/lb/MathGraphic.lh"
    return maxVal;
} else {
#line 69 "/usr/include/lb/MathGraphic.lh"
    return x;
}
}
}

#line 74 "/usr/include/lb/MathGraphic.lh"
float max(float a, float b) {
#line 75 "/usr/include/lb/MathGraphic.lh"
    if (((a>b))) {
#line 76 "/usr/include/lb/MathGraphic.lh"
    return a;
} else {
#line 78 "/usr/include/lb/MathGraphic.lh"
    return b;
}
}

#line 82 "/usr/include/lb/MathGraphic.lh"
vec2f v2(float x, float y) {
#line 83 "/usr/include/lb/MathGraphic.lh"
    vec2f v;
#line 84 "/usr/include/lb/MathGraphic.lh"
    v.x = x;
#line 85 "/usr/include/lb/MathGraphic.lh"
    v.y = y;
#line 86 "/usr/include/lb/MathGraphic.lh"
    return v;
}

#line 88 "/usr/include/lb/MathGraphic.lh"
vec3f v3(float x, float y, float z) {
#line 89 "/usr/include/lb/MathGraphic.lh"
    vec3f v;
#line 90 "/usr/include/lb/MathGraphic.lh"
    v.x = x;
#line 91 "/usr/include/lb/MathGraphic.lh"
    v.y = y;
#line 92 "/usr/include/lb/MathGraphic.lh"
    v.z = z;
#line 93 "/usr/include/lb/MathGraphic.lh"
    return v;
}

#line 95 "/usr/include/lb/MathGraphic.lh"
vec4f v4(float x, float y, float z, float w) {
#line 96 "/usr/include/lb/MathGraphic.lh"
    vec4f v;
#line 97 "/usr/include/lb/MathGraphic.lh"
    v.x = x;
#line 98 "/usr/include/lb/MathGraphic.lh"
    v.y = y;
#line 99 "/usr/include/lb/MathGraphic.lh"
    v.z = z;
#line 100 "/usr/include/lb/MathGraphic.lh"
    v.w = w;
#line 101 "/usr/include/lb/MathGraphic.lh"
    return v;
}

#line 103 "/usr/include/lb/MathGraphic.lh"
vec4f promote3f(vec3f v) {
#line 104 "/usr/include/lb/MathGraphic.lh"
    return v4(v.x, v.y, v.z, 1.0);
}

#line 106 "/usr/include/lb/MathGraphic.lh"
vec2f vec2Add(vec2f a, vec2f b) {
#line 107 "/usr/include/lb/MathGraphic.lh"
    return v2((a.x+b.x), (a.y+b.y));
}

#line 109 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Add(vec3f a, vec3f b) {
#line 110 "/usr/include/lb/MathGraphic.lh"
    return v3((a.x+b.x), (a.y+b.y), (a.z+b.z));
}

#line 112 "/usr/include/lb/MathGraphic.lh"
vec4f vec4Add(vec4f a, vec4f b) {
#line 113 "/usr/include/lb/MathGraphic.lh"
    return v4((a.x+b.x), (a.y+b.y), (a.z+b.z), (a.w+b.w));
}

#line 115 "/usr/include/lb/MathGraphic.lh"
vec2f vec2Sub(vec2f a, vec2f b) {
#line 116 "/usr/include/lb/MathGraphic.lh"
    return v2((a.x-b.x), (a.y-b.y));
}

#line 118 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Sub(vec3f a, vec3f b) {
#line 119 "/usr/include/lb/MathGraphic.lh"
    return v3((a.x-b.x), (a.y-b.y), (a.z-b.z));
}

#line 121 "/usr/include/lb/MathGraphic.lh"
vec4f vec4Sub(vec4f a, vec4f b) {
#line 122 "/usr/include/lb/MathGraphic.lh"
    return v4((a.x-b.x), (a.y-b.y), (a.z-b.z), (a.w-b.w));
}

#line 124 "/usr/include/lb/MathGraphic.lh"
vec2f vec2Mul(vec2f a, vec2f b) {
#line 125 "/usr/include/lb/MathGraphic.lh"
    return v2((a.x*b.x), (a.y*b.y));
}

#line 127 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Mul(vec3f a, vec3f b) {
#line 128 "/usr/include/lb/MathGraphic.lh"
    return v3((a.x*b.x), (a.y*b.y), (a.z*b.z));
}

#line 130 "/usr/include/lb/MathGraphic.lh"
vec4f vec4Mul(vec4f a, vec4f b) {
#line 131 "/usr/include/lb/MathGraphic.lh"
    return v4((a.x*b.x), (a.y*b.y), (a.z*b.z), (a.w*b.w));
}

#line 133 "/usr/include/lb/MathGraphic.lh"
vec2f vec2Div(vec2f a, vec2f b) {
#line 134 "/usr/include/lb/MathGraphic.lh"
    return v2((a.x/b.x), (a.y/b.y));
}

#line 136 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Div(vec3f a, vec3f b) {
#line 137 "/usr/include/lb/MathGraphic.lh"
    return v3((a.x/b.x), (a.y/b.y), (a.z/b.z));
}

#line 139 "/usr/include/lb/MathGraphic.lh"
vec4f vec4Div(vec4f a, vec4f b) {
#line 140 "/usr/include/lb/MathGraphic.lh"
    return v4((a.x/b.x), (a.y/b.y), (a.z/b.z), (a.w/b.w));
}

#line 142 "/usr/include/lb/MathGraphic.lh"
vec2f vec2Scale(vec2f v, float s) {
#line 143 "/usr/include/lb/MathGraphic.lh"
    return v2((v.x*s), (v.y*s));
}

#line 145 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Scale(vec3f v, float s) {
#line 146 "/usr/include/lb/MathGraphic.lh"
    return v3((v.x*s), (v.y*s), (v.z*s));
}

#line 148 "/usr/include/lb/MathGraphic.lh"
vec4f vec4Scale(vec4f v, float s) {
#line 149 "/usr/include/lb/MathGraphic.lh"
    return v4((v.x*s), (v.y*s), (v.z*s), (v.w*s));
}

#line 151 "/usr/include/lb/MathGraphic.lh"
float vec2Dot(vec2f a, vec2f b) {
#line 152 "/usr/include/lb/MathGraphic.lh"
    return ((a.x*b.x)+(a.y*b.y));
}

#line 154 "/usr/include/lb/MathGraphic.lh"
float vec3Dot(vec3f a, vec3f b) {
#line 155 "/usr/include/lb/MathGraphic.lh"
    return (((a.x*b.x)+(a.y*b.y))+(a.z*b.z));
}

#line 157 "/usr/include/lb/MathGraphic.lh"
float vec4Dot(vec4f a, vec4f b) {
#line 158 "/usr/include/lb/MathGraphic.lh"
    return ((((a.x*b.x)+(a.y*b.y))+(a.z*b.z))+(a.w*b.w));
}

#line 160 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Cross(vec3f a, vec3f b) {
#line 161 "/usr/include/lb/MathGraphic.lh"
    return v3(((a.y*b.z)-(a.z*b.y)), ((a.z*b.x)-(a.x*b.z)), ((a.x*b.y)-(a.y*b.x)));
}

#line 163 "/usr/include/lb/MathGraphic.lh"
float vec2Length(vec2f v) {
#line 164 "/usr/include/lb/MathGraphic.lh"
    return sqrt(((v.x*v.x)+(v.y*v.y)));
}

#line 166 "/usr/include/lb/MathGraphic.lh"
float vec3Length(vec3f v) {
#line 167 "/usr/include/lb/MathGraphic.lh"
    return sqrt((((v.x*v.x)+(v.y*v.y))+(v.z*v.z)));
}

#line 169 "/usr/include/lb/MathGraphic.lh"
float vec4Length(vec4f v) {
#line 170 "/usr/include/lb/MathGraphic.lh"
    return sqrt(((((v.x*v.x)+(v.y*v.y))+(v.z*v.z))+(v.w*v.w)));
}

#line 172 "/usr/include/lb/MathGraphic.lh"
vec2f vec2Normalize(vec2f v) {
#line 173 "/usr/include/lb/MathGraphic.lh"
    float len = vec2Length(v);
#line 174 "/usr/include/lb/MathGraphic.lh"
    if (((len>0.0))) {
#line 175 "/usr/include/lb/MathGraphic.lh"
    return vec2Scale(v, (1.0/len));
} else {
#line 177 "/usr/include/lb/MathGraphic.lh"
    return v2(0.0, 0.0);
}
}

#line 180 "/usr/include/lb/MathGraphic.lh"
vec3f vec3Normalize(vec3f v) {
#line 181 "/usr/include/lb/MathGraphic.lh"
    float len = vec3Length(v);
#line 182 "/usr/include/lb/MathGraphic.lh"
    if (((len>0.0))) {
#line 183 "/usr/include/lb/MathGraphic.lh"
    return vec3Scale(v, (1.0/len));
} else {
#line 185 "/usr/include/lb/MathGraphic.lh"
    return v3(0.0, 0.0, 0.0);
}
}

#line 188 "/usr/include/lb/MathGraphic.lh"
vec4f vec4Normalize(vec4f v) {
#line 189 "/usr/include/lb/MathGraphic.lh"
    float len = vec4Length(v);
#line 190 "/usr/include/lb/MathGraphic.lh"
    if (((len>0.0))) {
#line 191 "/usr/include/lb/MathGraphic.lh"
    return vec4Scale(v, (1.0/len));
} else {
#line 193 "/usr/include/lb/MathGraphic.lh"
    return v4(0.0, 0.0, 0.0, 0.0);
}
}

#line 197 "/usr/include/lb/MathGraphic.lh"
vec4f mulMat4Vec(mat4f m, vec4f v) {
#line 198 "/usr/include/lb/MathGraphic.lh"
    vec4f r;
#line 199 "/usr/include/lb/MathGraphic.lh"
    r.x = ((((m.x.x*v.x)+(m.y.x*v.y))+(m.z.x*v.z))+(m.w.x*v.w));
#line 200 "/usr/include/lb/MathGraphic.lh"
    r.y = ((((m.x.y*v.x)+(m.y.y*v.y))+(m.z.y*v.z))+(m.w.y*v.w));
#line 201 "/usr/include/lb/MathGraphic.lh"
    r.z = ((((m.x.z*v.x)+(m.y.z*v.y))+(m.z.z*v.z))+(m.w.z*v.w));
#line 202 "/usr/include/lb/MathGraphic.lh"
    r.w = ((((m.x.w*v.x)+(m.y.w*v.y))+(m.z.w*v.z))+(m.w.w*v.w));
#line 203 "/usr/include/lb/MathGraphic.lh"
    return r;
}

#line 205 "/usr/include/lb/MathGraphic.lh"
mat4f mulMat4Mat4(mat4f a, mat4f b) {
#line 206 "/usr/include/lb/MathGraphic.lh"
    mat4f r;
#line 207 "/usr/include/lb/MathGraphic.lh"
    r.x.x = ((((a.x.x*b.x.x)+(a.y.x*b.x.y))+(a.z.x*b.x.z))+(a.w.x*b.x.w));
#line 208 "/usr/include/lb/MathGraphic.lh"
    r.x.y = ((((a.x.y*b.x.x)+(a.y.y*b.x.y))+(a.z.y*b.x.z))+(a.w.y*b.x.w));
#line 209 "/usr/include/lb/MathGraphic.lh"
    r.x.z = ((((a.x.z*b.x.x)+(a.y.z*b.x.y))+(a.z.z*b.x.z))+(a.w.z*b.x.w));
#line 210 "/usr/include/lb/MathGraphic.lh"
    r.x.w = ((((a.x.w*b.x.x)+(a.y.w*b.x.y))+(a.z.w*b.x.z))+(a.w.w*b.x.w));
#line 211 "/usr/include/lb/MathGraphic.lh"
    r.y.x = ((((a.x.x*b.y.x)+(a.y.x*b.y.y))+(a.z.x*b.y.z))+(a.w.x*b.y.w));
#line 212 "/usr/include/lb/MathGraphic.lh"
    r.y.y = ((((a.x.y*b.y.x)+(a.y.y*b.y.y))+(a.z.y*b.y.z))+(a.w.y*b.y.w));
#line 213 "/usr/include/lb/MathGraphic.lh"
    r.y.z = ((((a.x.z*b.y.x)+(a.y.z*b.y.y))+(a.z.z*b.y.z))+(a.w.z*b.y.w));
#line 214 "/usr/include/lb/MathGraphic.lh"
    r.y.w = ((((a.x.w*b.y.x)+(a.y.w*b.y.y))+(a.z.w*b.y.z))+(a.w.w*b.y.w));
#line 215 "/usr/include/lb/MathGraphic.lh"
    r.z.x = ((((a.x.x*b.z.x)+(a.y.x*b.z.y))+(a.z.x*b.z.z))+(a.w.x*b.z.w));
#line 216 "/usr/include/lb/MathGraphic.lh"
    r.z.y = ((((a.x.y*b.z.x)+(a.y.y*b.z.y))+(a.z.y*b.z.z))+(a.w.y*b.z.w));
#line 217 "/usr/include/lb/MathGraphic.lh"
    r.z.z = ((((a.x.z*b.z.x)+(a.y.z*b.z.y))+(a.z.z*b.z.z))+(a.w.z*b.z.w));
#line 218 "/usr/include/lb/MathGraphic.lh"
    r.z.w = ((((a.x.w*b.z.x)+(a.y.w*b.z.y))+(a.z.w*b.z.z))+(a.w.w*b.z.w));
#line 219 "/usr/include/lb/MathGraphic.lh"
    r.w.x = ((((a.x.x*b.w.x)+(a.y.x*b.w.y))+(a.z.x*b.w.z))+(a.w.x*b.w.w));
#line 220 "/usr/include/lb/MathGraphic.lh"
    r.w.y = ((((a.x.y*b.w.x)+(a.y.y*b.w.y))+(a.z.y*b.w.z))+(a.w.y*b.w.w));
#line 221 "/usr/include/lb/MathGraphic.lh"
    r.w.z = ((((a.x.z*b.w.x)+(a.y.z*b.w.y))+(a.z.z*b.w.z))+(a.w.z*b.w.w));
#line 222 "/usr/include/lb/MathGraphic.lh"
    r.w.w = ((((a.x.w*b.w.x)+(a.y.w*b.w.y))+(a.z.w*b.w.z))+(a.w.w*b.w.w));
#line 223 "/usr/include/lb/MathGraphic.lh"
    return r;
}

#line 225 "/usr/include/lb/MathGraphic.lh"
quatf mulQuatQuat(quatf a, quatf b) {
#line 226 "/usr/include/lb/MathGraphic.lh"
    quatf q;
#line 227 "/usr/include/lb/MathGraphic.lh"
    q.x = ((((a.w*b.x)+(a.x*b.w))+(a.y*b.z))-(a.z*b.y));
#line 228 "/usr/include/lb/MathGraphic.lh"
    q.y = ((((a.w*b.y)-(a.x*b.z))+(a.y*b.w))+(a.z*b.x));
#line 229 "/usr/include/lb/MathGraphic.lh"
    q.z = ((((a.w*b.z)+(a.x*b.y))-(a.y*b.x))+(a.z*b.w));
#line 230 "/usr/include/lb/MathGraphic.lh"
    q.w = ((((a.w*b.w)-(a.x*b.x))-(a.y*b.y))-(a.z*b.z));
#line 231 "/usr/include/lb/MathGraphic.lh"
    return q;
}

#line 233 "/usr/include/lb/MathGraphic.lh"
quatf mulQuatVec(quatf q, vec3f v) {
#line 234 "/usr/include/lb/MathGraphic.lh"
    quatf p;
#line 235 "/usr/include/lb/MathGraphic.lh"
    p.x = v.x;
#line 236 "/usr/include/lb/MathGraphic.lh"
    p.y = v.y;
#line 237 "/usr/include/lb/MathGraphic.lh"
    p.z = v.z;
#line 238 "/usr/include/lb/MathGraphic.lh"
    p.w = 0.0;
#line 239 "/usr/include/lb/MathGraphic.lh"
    quatf q_conjugate;
#line 240 "/usr/include/lb/MathGraphic.lh"
    q_conjugate.x = (-q.x);
#line 241 "/usr/include/lb/MathGraphic.lh"
    q_conjugate.y = (-q.y);
#line 242 "/usr/include/lb/MathGraphic.lh"
    q_conjugate.z = (-q.z);
#line 243 "/usr/include/lb/MathGraphic.lh"
    q_conjugate.w = q.w;
#line 244 "/usr/include/lb/MathGraphic.lh"
    quatf r = mulQuatQuat(mulQuatQuat(q, p), q_conjugate);
#line 245 "/usr/include/lb/MathGraphic.lh"
    return r;
}

#line 261 "/usr/include/lb/MathGraphic.lh"
mat4f identity() {
#line 262 "/usr/include/lb/MathGraphic.lh"
    mat4f m;
#line 263 "/usr/include/lb/MathGraphic.lh"
    m.x = v4(1.0, 0.0, 0.0, 0.0);
#line 264 "/usr/include/lb/MathGraphic.lh"
    m.y = v4(0.0, 1.0, 0.0, 0.0);
#line 265 "/usr/include/lb/MathGraphic.lh"
    m.z = v4(0.0, 0.0, 1.0, 0.0);
#line 266 "/usr/include/lb/MathGraphic.lh"
    m.w = v4(0.0, 0.0, 0.0, 1.0);
#line 267 "/usr/include/lb/MathGraphic.lh"
    return m;
}

#line 269 "/usr/include/lb/MathGraphic.lh"
mat4f translate(mat4f m, vec3f v) {
#line 270 "/usr/include/lb/MathGraphic.lh"
    mat4f t = identity();
#line 271 "/usr/include/lb/MathGraphic.lh"
    t.w = v4(v.x, v.y, v.z, 1.0);
#line 272 "/usr/include/lb/MathGraphic.lh"
    return mulMat4Mat4(m, t);
}

#line 274 "/usr/include/lb/MathGraphic.lh"
mat4f rotate(mat4f m, float angle, vec3f axis) {
#line 275 "/usr/include/lb/MathGraphic.lh"
    float c = cos(angle);
#line 276 "/usr/include/lb/MathGraphic.lh"
    float s = sin(angle);
#line 277 "/usr/include/lb/MathGraphic.lh"
    float t = (1.0-c);
#line 278 "/usr/include/lb/MathGraphic.lh"
    vec3f n = vec3Normalize(axis);
#line 279 "/usr/include/lb/MathGraphic.lh"
    mat4f r;
#line 280 "/usr/include/lb/MathGraphic.lh"
    r.x = v4((((t*n.x)*n.x)+c), (((t*n.x)*n.y)-(s*n.z)), (((t*n.x)*n.z)+(s*n.y)), 0.0);
#line 281 "/usr/include/lb/MathGraphic.lh"
    r.y = v4((((t*n.x)*n.y)+(s*n.z)), (((t*n.y)*n.y)+c), (((t*n.y)*n.z)-(s*n.x)), 0.0);
#line 282 "/usr/include/lb/MathGraphic.lh"
    r.z = v4((((t*n.x)*n.z)-(s*n.y)), (((t*n.y)*n.z)+(s*n.x)), (((t*n.z)*n.z)+c), 0.0);
#line 283 "/usr/include/lb/MathGraphic.lh"
    r.w = v4(0.0, 0.0, 0.0, 1.0);
#line 284 "/usr/include/lb/MathGraphic.lh"
    return mulMat4Mat4(m, r);
}

#line 286 "/usr/include/lb/MathGraphic.lh"
mat4f scale(mat4f m, vec3f v) {
#line 287 "/usr/include/lb/MathGraphic.lh"
    mat4f s = identity();
#line 288 "/usr/include/lb/MathGraphic.lh"
    s.x.x = v.x;
#line 289 "/usr/include/lb/MathGraphic.lh"
    s.y.y = v.y;
#line 290 "/usr/include/lb/MathGraphic.lh"
    s.z.z = v.z;
#line 291 "/usr/include/lb/MathGraphic.lh"
    return mulMat4Mat4(m, s);
}

#line 293 "/usr/include/lb/MathGraphic.lh"
quatf fromAxisAngle(vec3f axis, float angle) {
#line 294 "/usr/include/lb/MathGraphic.lh"
    float s = sin((angle*0.5));
#line 295 "/usr/include/lb/MathGraphic.lh"
    quatf q;
#line 296 "/usr/include/lb/MathGraphic.lh"
    q.x = (axis.x*s);
#line 297 "/usr/include/lb/MathGraphic.lh"
    q.y = (axis.y*s);
#line 298 "/usr/include/lb/MathGraphic.lh"
    q.z = (axis.z*s);
#line 299 "/usr/include/lb/MathGraphic.lh"
    q.w = cos((angle*0.5));
#line 300 "/usr/include/lb/MathGraphic.lh"
    return q;
}

#line 321 "/usr/include/lb/MathGraphic.lh"
mat4f perspective(float fov, float aspect, float near, float far) {
#line 322 "/usr/include/lb/MathGraphic.lh"
    float tanHalfFov = tan((fov*0.5));
#line 323 "/usr/include/lb/MathGraphic.lh"
    mat4f m;
#line 324 "/usr/include/lb/MathGraphic.lh"
    m.x = v4((1.0/((aspect*tanHalfFov))), 0.0, 0.0, 0.0);
#line 325 "/usr/include/lb/MathGraphic.lh"
    m.y = v4(0.0, (1.0/tanHalfFov), 0.0, 0.0);
#line 326 "/usr/include/lb/MathGraphic.lh"
    m.z = v4(0.0, 0.0, ((-((far+near)))/((far-near))), (-1.0));
#line 327 "/usr/include/lb/MathGraphic.lh"
    m.w = v4(0.0, 0.0, ((-(((2.0*far)*near)))/((far-near))), 0.0);
#line 328 "/usr/include/lb/MathGraphic.lh"
    return m;
}

#line 330 "/usr/include/lb/MathGraphic.lh"
mat4f lookAt(vec3f eye, vec3f center, vec3f up) {
#line 331 "/usr/include/lb/MathGraphic.lh"
    vec3f f = vec3Normalize(vec3Sub(center, eye));
#line 332 "/usr/include/lb/MathGraphic.lh"
    vec3f s = vec3Normalize(vec3Cross(f, up));
#line 333 "/usr/include/lb/MathGraphic.lh"
    vec3f u = vec3Cross(s, f);
#line 334 "/usr/include/lb/MathGraphic.lh"
    mat4f m;
#line 335 "/usr/include/lb/MathGraphic.lh"
    m.x = v4(s.x, u.x, (-f.x), 0.0);
#line 336 "/usr/include/lb/MathGraphic.lh"
    m.y = v4(s.y, u.y, (-f.y), 0.0);
#line 337 "/usr/include/lb/MathGraphic.lh"
    m.z = v4(s.z, u.z, (-f.z), 0.0);
#line 338 "/usr/include/lb/MathGraphic.lh"
    m.w = v4((-vec3Dot(s, eye)), (-vec3Dot(u, eye)), vec3Dot(f, eye), 1.0);
#line 339 "/usr/include/lb/MathGraphic.lh"
    return m;
}

#line 341 "/usr/include/lb/MathGraphic.lh"
mat4f ortho(float left, float right, float bottom, float top, float near, float far) {
#line 342 "/usr/include/lb/MathGraphic.lh"
    mat4f m;
#line 343 "/usr/include/lb/MathGraphic.lh"
    m.x = v4((2.0/((right-left))), 0.0, 0.0, 0.0);
#line 344 "/usr/include/lb/MathGraphic.lh"
    m.y = v4(0.0, (2.0/((top-bottom))), 0.0, 0.0);
#line 345 "/usr/include/lb/MathGraphic.lh"
    m.z = v4(0.0, 0.0, ((-2.0)/((far-near))), 0.0);
#line 346 "/usr/include/lb/MathGraphic.lh"
    m.w = v4(((-((right+left)))/((right-left))), ((-((top+bottom)))/((top-bottom))), ((-((far+near)))/((far-near))), 1.0);
#line 347 "/usr/include/lb/MathGraphic.lh"
    return m;
}

#line 349 "/usr/include/lb/MathGraphic.lh"
mat4f inverse(mat4f m) {
#line 350 "/usr/include/lb/MathGraphic.lh"
    mat4f inv;
#line 351 "/usr/include/lb/MathGraphic.lh"
    float det;
#line 352 "/usr/include/lb/MathGraphic.lh"
    int i;
#line 353 "/usr/include/lb/MathGraphic.lh"
    inv.x.x = (((((((m.y.y*m.z.z)*m.w.w)-((m.y.y*m.z.w)*m.w.z))-((m.z.y*m.y.z)*m.w.w))+((m.z.y*m.y.w)*m.w.z))+((m.w.y*m.y.z)*m.z.w))-((m.w.y*m.y.w)*m.z.z));
#line 354 "/usr/include/lb/MathGraphic.lh"
    inv.x.y = ((((((((-m.y.x)*m.z.z)*m.w.w)+((m.y.x*m.z.w)*m.w.z))+((m.z.x*m.y.z)*m.w.w))-((m.z.x*m.y.w)*m.w.z))-((m.w.x*m.y.z)*m.z.w))+((m.w.x*m.y.w)*m.z.z));
#line 355 "/usr/include/lb/MathGraphic.lh"
    inv.x.z = (((((((m.y.x*m.z.y)*m.w.w)-((m.y.x*m.z.w)*m.w.y))-((m.z.x*m.y.y)*m.w.w))+((m.z.x*m.y.w)*m.w.y))+((m.w.x*m.y.y)*m.z.w))-((m.w.x*m.y.w)*m.z.y));
#line 356 "/usr/include/lb/MathGraphic.lh"
    inv.x.w = ((((((((-m.y.x)*m.z.y)*m.w.z)+((m.y.x*m.z.z)*m.w.y))+((m.z.x*m.y.y)*m.w.z))-((m.z.x*m.y.z)*m.w.y))-((m.w.x*m.y.y)*m.z.z))+((m.w.x*m.y.z)*m.z.y));
#line 357 "/usr/include/lb/MathGraphic.lh"
    inv.y.x = ((((((((-m.x.y)*m.z.z)*m.w.w)+((m.x.y*m.z.w)*m.w.z))+((m.z.y*m.x.z)*m.w.w))-((m.z.y*m.x.w)*m.w.z))-((m.w.y*m.x.z)*m.z.w))+((m.w.y*m.x.w)*m.z.z));
#line 358 "/usr/include/lb/MathGraphic.lh"
    inv.y.y = (((((((m.x.x*m.z.z)*m.w.w)-((m.x.x*m.z.w)*m.w.z))-((m.z.x*m.x.z)*m.w.w))+((m.z.x*m.x.w)*m.w.z))+((m.w.x*m.x.z)*m.z.w))-((m.w.x*m.x.w)*m.z.z));
#line 359 "/usr/include/lb/MathGraphic.lh"
    inv.y.z = ((((((((-m.x.x)*m.z.y)*m.w.w)+((m.x.x*m.z.w)*m.w.y))+((m.z.x*m.x.y)*m.w.w))-((m.z.x*m.x.w)*m.w.y))-((m.w.x*m.x.y)*m.z.w))+((m.w.x*m.x.w)*m.z.y));
#line 360 "/usr/include/lb/MathGraphic.lh"
    inv.y.w = (((((((m.x.x*m.z.y)*m.w.z)-((m.x.x*m.z.z)*m.w.y))-((m.z.x*m.x.y)*m.w.z))+((m.z.x*m.x.z)*m.w.y))+((m.w.x*m.x.y)*m.z.z))-((m.w.x*m.x.z)*m.z.y));
#line 361 "/usr/include/lb/MathGraphic.lh"
    inv.z.x = (((((((m.x.y*m.y.z)*m.w.w)-((m.x.y*m.y.w)*m.w.z))-((m.y.y*m.x.z)*m.w.w))+((m.y.y*m.x.w)*m.w.z))+((m.w.y*m.x.z)*m.y.w))-((m.w.y*m.x.w)*m.y.z));
#line 362 "/usr/include/lb/MathGraphic.lh"
    inv.z.y = ((((((((-m.x.x)*m.y.z)*m.w.w)+((m.x.x*m.y.w)*m.w.z))+((m.y.x*m.x.z)*m.w.w))-((m.y.x*m.x.w)*m.w.z))-((m.w.x*m.x.z)*m.y.w))+((m.w.x*m.x.w)*m.y.z));
#line 363 "/usr/include/lb/MathGraphic.lh"
    inv.z.z = (((((((m.x.x*m.y.y)*m.w.w)-((m.x.x*m.y.w)*m.w.y))-((m.y.x*m.x.y)*m.w.w))+((m.y.x*m.x.w)*m.w.y))+((m.w.x*m.x.y)*m.y.w))-((m.w.x*m.x.w)*m.y.y));
#line 364 "/usr/include/lb/MathGraphic.lh"
    inv.z.w = ((((((((-m.x.x)*m.y.y)*m.w.z)+((m.x.x*m.y.z)*m.w.y))+((m.y.x*m.x.y)*m.w.z))-((m.y.x*m.x.z)*m.w.y))-((m.w.x*m.x.y)*m.y.z))+((m.w.x*m.x.z)*m.y.y));
#line 365 "/usr/include/lb/MathGraphic.lh"
    inv.w.x = ((((((((-m.x.y)*m.y.z)*m.z.w)+((m.x.y*m.y.w)*m.z.z))+((m.y.y*m.x.z)*m.z.w))-((m.y.y*m.x.w)*m.z.z))-((m.z.y*m.x.z)*m.y.w))+((m.z.y*m.x.w)*m.y.z));
#line 366 "/usr/include/lb/MathGraphic.lh"
    inv.w.y = (((((((m.x.x*m.y.z)*m.z.w)-((m.x.x*m.y.w)*m.z.z))-((m.y.x*m.x.z)*m.z.w))+((m.y.x*m.x.w)*m.z.z))+((m.z.x*m.x.z)*m.y.w))-((m.z.x*m.x.w)*m.y.z));
#line 367 "/usr/include/lb/MathGraphic.lh"
    inv.w.z = ((((((((-m.x.x)*m.y.y)*m.z.w)+((m.x.x*m.y.w)*m.z.y))+((m.y.x*m.x.y)*m.z.w))-((m.y.x*m.x.w)*m.z.y))-((m.z.x*m.x.y)*m.y.w))+((m.z.x*m.x.w)*m.y.y));
#line 368 "/usr/include/lb/MathGraphic.lh"
    inv.w.w = (((((((m.x.x*m.y.y)*m.z.z)-((m.x.x*m.y.z)*m.z.y))-((m.y.x*m.x.y)*m.z.z))+((m.y.x*m.x.z)*m.z.y))+((m.z.x*m.x.y)*m.y.z))-((m.z.x*m.x.z)*m.y.y));
#line 369 "/usr/include/lb/MathGraphic.lh"
    det = ((((m.x.x*inv.x.x)+(m.x.y*inv.x.y))+(m.x.z*inv.x.z))+(m.x.w*inv.x.w));
#line 370 "/usr/include/lb/MathGraphic.lh"
    if (((det==0.0))) {
#line 371 "/usr/include/lb/MathGraphic.lh"
    return identity();
}
#line 373 "/usr/include/lb/MathGraphic.lh"
    det = (1.0/det);
#line 374 "/usr/include/lb/MathGraphic.lh"
    inv.x.x = (inv.x.x*det);
#line 375 "/usr/include/lb/MathGraphic.lh"
    inv.x.y = (inv.x.y*det);
#line 376 "/usr/include/lb/MathGraphic.lh"
    inv.x.z = (inv.x.z*det);
#line 377 "/usr/include/lb/MathGraphic.lh"
    inv.x.w = (inv.x.w*det);
#line 378 "/usr/include/lb/MathGraphic.lh"
    inv.y.x = (inv.y.x*det);
#line 379 "/usr/include/lb/MathGraphic.lh"
    inv.y.y = (inv.y.y*det);
#line 380 "/usr/include/lb/MathGraphic.lh"
    inv.y.z = (inv.y.z*det);
#line 381 "/usr/include/lb/MathGraphic.lh"
    inv.y.w = (inv.y.w*det);
#line 382 "/usr/include/lb/MathGraphic.lh"
    inv.z.x = (inv.z.x*det);
#line 383 "/usr/include/lb/MathGraphic.lh"
    inv.z.y = (inv.z.y*det);
#line 384 "/usr/include/lb/MathGraphic.lh"
    inv.z.z = (inv.z.z*det);
#line 385 "/usr/include/lb/MathGraphic.lh"
    inv.z.w = (inv.z.w*det);
#line 386 "/usr/include/lb/MathGraphic.lh"
    inv.w.x = (inv.w.x*det);
#line 387 "/usr/include/lb/MathGraphic.lh"
    inv.w.y = (inv.w.y*det);
#line 388 "/usr/include/lb/MathGraphic.lh"
    inv.w.z = (inv.w.z*det);
#line 389 "/usr/include/lb/MathGraphic.lh"
    inv.w.w = (inv.w.w*det);
#line 390 "/usr/include/lb/MathGraphic.lh"
    return inv;
}

#line 392 "/usr/include/lb/MathGraphic.lh"
mat4f transpose(mat4f m) {
#line 393 "/usr/include/lb/MathGraphic.lh"
    mat4f t;
#line 394 "/usr/include/lb/MathGraphic.lh"
    t.x = v4(m.x.x, m.y.x, m.z.x, m.w.x);
#line 395 "/usr/include/lb/MathGraphic.lh"
    t.y = v4(m.x.y, m.y.y, m.z.y, m.w.y);
#line 396 "/usr/include/lb/MathGraphic.lh"
    t.z = v4(m.x.z, m.y.z, m.z.z, m.w.z);
#line 397 "/usr/include/lb/MathGraphic.lh"
    t.w = v4(m.x.w, m.y.w, m.z.w, m.w.w);
#line 398 "/usr/include/lb/MathGraphic.lh"
    return t;
}

#line 400 "/usr/include/lb/MathGraphic.lh"
quatf fromEuler(vec3f euler) {
#line 401 "/usr/include/lb/MathGraphic.lh"
    float c1 = cos((euler.y*0.5));
#line 402 "/usr/include/lb/MathGraphic.lh"
    float s1 = sin((euler.y*0.5));
#line 403 "/usr/include/lb/MathGraphic.lh"
    float c2 = cos((euler.x*0.5));
#line 404 "/usr/include/lb/MathGraphic.lh"
    float s2 = sin((euler.x*0.5));
#line 405 "/usr/include/lb/MathGraphic.lh"
    float c3 = cos((euler.z*0.5));
#line 406 "/usr/include/lb/MathGraphic.lh"
    float s3 = sin((euler.z*0.5));
#line 407 "/usr/include/lb/MathGraphic.lh"
    quatf q;
#line 408 "/usr/include/lb/MathGraphic.lh"
    q.x = (((s1*s2)*c3)+((c1*c2)*s3));
#line 409 "/usr/include/lb/MathGraphic.lh"
    q.y = (((s1*c2)*c3)+((c1*s2)*s3));
#line 410 "/usr/include/lb/MathGraphic.lh"
    q.z = (((c1*s2)*c3)-((s1*c2)*s3));
#line 411 "/usr/include/lb/MathGraphic.lh"
    q.w = (((c1*c2)*c3)-((s1*s2)*s3));
#line 412 "/usr/include/lb/MathGraphic.lh"
    return q;
}

#line 414 "/usr/include/lb/MathGraphic.lh"
vec3f toEuler(quatf q) {
#line 415 "/usr/include/lb/MathGraphic.lh"
    vec3f euler;
#line 416 "/usr/include/lb/MathGraphic.lh"
    float sinr_cosp = (2.0*(((q.w*q.x)+(q.y*q.z))));
#line 417 "/usr/include/lb/MathGraphic.lh"
    float cosr_cosp = (1.0-(2.0*(((q.x*q.x)+(q.y*q.y)))));
#line 418 "/usr/include/lb/MathGraphic.lh"
    euler.x = atan2(sinr_cosp, cosr_cosp);
#line 419 "/usr/include/lb/MathGraphic.lh"
    float sinp = (2.0*(((q.w*q.y)-(q.z*q.x))));
#line 420 "/usr/include/lb/MathGraphic.lh"
    if (((fabsf(sinp)>=1.0))) {
#line 421 "/usr/include/lb/MathGraphic.lh"
    euler.y = copysign(1.5707963267948966, sinp);
} else {
#line 423 "/usr/include/lb/MathGraphic.lh"
    euler.y = asin(sinp);
}
#line 425 "/usr/include/lb/MathGraphic.lh"
    float siny_cosp = (2.0*(((q.w*q.z)+(q.x*q.y))));
#line 426 "/usr/include/lb/MathGraphic.lh"
    float cosy_cosp = (1.0-(2.0*(((q.y*q.y)+(q.z*q.z)))));
#line 427 "/usr/include/lb/MathGraphic.lh"
    euler.z = atan2(siny_cosp, cosy_cosp);
#line 428 "/usr/include/lb/MathGraphic.lh"
    return euler;
}

#line 430 "/usr/include/lb/MathGraphic.lh"
quatf slerp(quatf a, quatf b, float t) {
#line 431 "/usr/include/lb/MathGraphic.lh"
    float cosTheta = ((((a.x*b.x)+(a.y*b.y))+(a.z*b.z))+(a.w*b.w));
#line 432 "/usr/include/lb/MathGraphic.lh"
    if (((cosTheta<0.0))) {
#line 433 "/usr/include/lb/MathGraphic.lh"
    b.x = (-b.x);
#line 434 "/usr/include/lb/MathGraphic.lh"
    b.y = (-b.y);
#line 435 "/usr/include/lb/MathGraphic.lh"
    b.z = (-b.z);
#line 436 "/usr/include/lb/MathGraphic.lh"
    b.w = (-b.w);
#line 437 "/usr/include/lb/MathGraphic.lh"
    cosTheta = (-cosTheta);
}
#line 439 "/usr/include/lb/MathGraphic.lh"
    float angle = acos(cosTheta);
#line 440 "/usr/include/lb/MathGraphic.lh"
    if (((angle<0.001))) {
#line 441 "/usr/include/lb/MathGraphic.lh"
    return a;
}
#line 443 "/usr/include/lb/MathGraphic.lh"
    float sinTheta = sin(angle);
#line 444 "/usr/include/lb/MathGraphic.lh"
    float weightA = (sin((((1.0-t))*angle))/sinTheta);
#line 445 "/usr/include/lb/MathGraphic.lh"
    float weightB = (sin((t*angle))/sinTheta);
#line 446 "/usr/include/lb/MathGraphic.lh"
    quatf q;
#line 447 "/usr/include/lb/MathGraphic.lh"
    q.x = ((a.x*weightA)+(b.x*weightB));
#line 448 "/usr/include/lb/MathGraphic.lh"
    q.y = ((a.y*weightA)+(b.y*weightB));
#line 449 "/usr/include/lb/MathGraphic.lh"
    q.z = ((a.z*weightA)+(b.z*weightB));
#line 450 "/usr/include/lb/MathGraphic.lh"
    q.w = ((a.w*weightA)+(b.w*weightB));
#line 451 "/usr/include/lb/MathGraphic.lh"
    return q;
}

#line 453 "/usr/include/lb/MathGraphic.lh"
quatf fromMat4(mat4f m) {
#line 454 "/usr/include/lb/MathGraphic.lh"
    quatf q;
#line 455 "/usr/include/lb/MathGraphic.lh"
    float trace = ((m.x.x+m.y.y)+m.z.z);
#line 456 "/usr/include/lb/MathGraphic.lh"
    if (((trace>0.0))) {
#line 457 "/usr/include/lb/MathGraphic.lh"
    float s = (0.5/sqrt((trace+1.0)));
#line 458 "/usr/include/lb/MathGraphic.lh"
    q.w = (0.25/s);
#line 459 "/usr/include/lb/MathGraphic.lh"
    q.x = (((m.y.z-m.z.y))*s);
#line 460 "/usr/include/lb/MathGraphic.lh"
    q.y = (((m.z.x-m.x.z))*s);
#line 461 "/usr/include/lb/MathGraphic.lh"
    q.z = (((m.x.y-m.y.x))*s);
} else if ((((m.x.x>m.y.y) && (m.x.x>m.z.z)))) {
#line 463 "/usr/include/lb/MathGraphic.lh"
    float s = (2.0*sqrt((((1.0+m.x.x)-m.y.y)-m.z.z)));
#line 464 "/usr/include/lb/MathGraphic.lh"
    q.w = (((m.y.z-m.z.y))/s);
#line 465 "/usr/include/lb/MathGraphic.lh"
    q.x = (0.25*s);
#line 466 "/usr/include/lb/MathGraphic.lh"
    q.y = (((m.y.x+m.x.y))/s);
#line 467 "/usr/include/lb/MathGraphic.lh"
    q.z = (((m.z.x+m.x.z))/s);
} else if (((m.y.y>m.z.z))) {
#line 469 "/usr/include/lb/MathGraphic.lh"
    float s = (2.0*sqrt((((1.0+m.y.y)-m.x.x)-m.z.z)));
#line 470 "/usr/include/lb/MathGraphic.lh"
    q.w = (((m.z.x-m.x.z))/s);
#line 471 "/usr/include/lb/MathGraphic.lh"
    q.x = (((m.y.x+m.x.y))/s);
#line 472 "/usr/include/lb/MathGraphic.lh"
    q.y = (0.25*s);
#line 473 "/usr/include/lb/MathGraphic.lh"
    q.z = (((m.z.y+m.y.z))/s);
} else {
#line 475 "/usr/include/lb/MathGraphic.lh"
    float s = (2.0*sqrt((((1.0+m.z.z)-m.x.x)-m.y.y)));
#line 476 "/usr/include/lb/MathGraphic.lh"
    q.w = (((m.x.y-m.y.x))/s);
#line 477 "/usr/include/lb/MathGraphic.lh"
    q.x = (((m.z.x+m.x.z))/s);
#line 478 "/usr/include/lb/MathGraphic.lh"
    q.y = (((m.z.y+m.y.z))/s);
#line 479 "/usr/include/lb/MathGraphic.lh"
    q.z = (0.25*s);
}
#line 481 "/usr/include/lb/MathGraphic.lh"
    return q;
}

#line 483 "/usr/include/lb/MathGraphic.lh"
mat4f fromQuat(quatf q) {
#line 484 "/usr/include/lb/MathGraphic.lh"
    mat4f m;
#line 485 "/usr/include/lb/MathGraphic.lh"
    float xx = (q.x*q.x);
#line 486 "/usr/include/lb/MathGraphic.lh"
    float yy = (q.y*q.y);
#line 487 "/usr/include/lb/MathGraphic.lh"
    float zz = (q.z*q.z);
#line 488 "/usr/include/lb/MathGraphic.lh"
    float xy = (q.x*q.y);
#line 489 "/usr/include/lb/MathGraphic.lh"
    float xz = (q.x*q.z);
#line 490 "/usr/include/lb/MathGraphic.lh"
    float yz = (q.y*q.z);
#line 491 "/usr/include/lb/MathGraphic.lh"
    float wx = (q.w*q.x);
#line 492 "/usr/include/lb/MathGraphic.lh"
    float wy = (q.w*q.y);
#line 493 "/usr/include/lb/MathGraphic.lh"
    float wz = (q.w*q.z);
#line 494 "/usr/include/lb/MathGraphic.lh"
    m.x = v4((1.0-(2.0*((yy+zz)))), (2.0*((xy+wz))), (2.0*((xz-wy))), 0.0);
#line 495 "/usr/include/lb/MathGraphic.lh"
    m.y = v4((2.0*((xy-wz))), (1.0-(2.0*((xx+zz)))), (2.0*((yz+wx))), 0.0);
#line 496 "/usr/include/lb/MathGraphic.lh"
    m.z = v4((2.0*((xz+wy))), (2.0*((yz-wx))), (1.0-(2.0*((xx+yy)))), 0.0);
#line 497 "/usr/include/lb/MathGraphic.lh"
    m.w = v4(0.0, 0.0, 0.0, 1.0);
#line 498 "/usr/include/lb/MathGraphic.lh"
    return m;
}

#line 500 "/usr/include/lb/MathGraphic.lh"
vec3f project(vec3f point, mat4f model, mat4f view, mat4f projection) {
#line 501 "/usr/include/lb/MathGraphic.lh"
    vec4f clipSpace = mulMat4Vec(projection, mulMat4Vec(view, mulMat4Vec(model, v4(point.x, point.y, point.z, 1.0))));
#line 502 "/usr/include/lb/MathGraphic.lh"
    vec3f screenSpace;
#line 503 "/usr/include/lb/MathGraphic.lh"
    screenSpace.x = (clipSpace.x/clipSpace.w);
#line 504 "/usr/include/lb/MathGraphic.lh"
    screenSpace.y = (clipSpace.y/clipSpace.w);
#line 505 "/usr/include/lb/MathGraphic.lh"
    screenSpace.z = (clipSpace.z/clipSpace.w);
#line 506 "/usr/include/lb/MathGraphic.lh"
    return screenSpace;
}

#line 508 "/usr/include/lb/MathGraphic.lh"
vec3f unproject(vec3f point, mat4f model, mat4f view, mat4f projection) {
#line 509 "/usr/include/lb/MathGraphic.lh"
    mat4f inv = inverse(mulMat4Mat4(projection, mulMat4Mat4(view, model)));
#line 510 "/usr/include/lb/MathGraphic.lh"
    vec4f worldSpace = mulMat4Vec(inv, v4(point.x, point.y, point.z, 1.0));
#line 511 "/usr/include/lb/MathGraphic.lh"
    if (((worldSpace.w!=0.0))) {
#line 512 "/usr/include/lb/MathGraphic.lh"
    worldSpace.x /= worldSpace.w;
#line 513 "/usr/include/lb/MathGraphic.lh"
    worldSpace.y /= worldSpace.w;
#line 514 "/usr/include/lb/MathGraphic.lh"
    worldSpace.z /= worldSpace.w;
}
#line 516 "/usr/include/lb/MathGraphic.lh"
    return v3(worldSpace.x, worldSpace.y, worldSpace.z);
}

#line 518 "/usr/include/lb/MathGraphic.lh"
rayf screenPointToRay(vec2f screenPoint, mat4f view, mat4f projection, vec2f screenSize) {
#line 519 "/usr/include/lb/MathGraphic.lh"
    vec2f ndc;
#line 520 "/usr/include/lb/MathGraphic.lh"
    ndc.x = ((((2.0*screenPoint.x))/screenSize.x)-1.0);
#line 521 "/usr/include/lb/MathGraphic.lh"
    ndc.y = (1.0-(((2.0*screenPoint.y))/screenSize.y));
#line 522 "/usr/include/lb/MathGraphic.lh"
    vec4f clipSpace = v4(ndc.x, ndc.y, (-1.0), 1.0);
#line 523 "/usr/include/lb/MathGraphic.lh"
    mat4f invProj = inverse(projection);
#line 524 "/usr/include/lb/MathGraphic.lh"
    vec4f eyeSpace = mulMat4Vec(invProj, clipSpace);
#line 525 "/usr/include/lb/MathGraphic.lh"
    eyeSpace.z = (-1.0);
#line 526 "/usr/include/lb/MathGraphic.lh"
    eyeSpace.w = 0.0;
#line 527 "/usr/include/lb/MathGraphic.lh"
    mat4f invView = inverse(view);
#line 528 "/usr/include/lb/MathGraphic.lh"
    vec4f worldSpace = mulMat4Vec(invView, eyeSpace);
#line 529 "/usr/include/lb/MathGraphic.lh"
    rayf ray;
#line 530 "/usr/include/lb/MathGraphic.lh"
    ray.origin = v3(invView.w.x, invView.w.y, invView.w.z);
#line 531 "/usr/include/lb/MathGraphic.lh"
    ray.direction = vec3Normalize(v3(worldSpace.x, worldSpace.y, worldSpace.z));
#line 532 "/usr/include/lb/MathGraphic.lh"
    return ray;
}

#line 534 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRaySphere(rayf ray, spheref sphere) {
#line 535 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 536 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 537 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 538 "/usr/include/lb/MathGraphic.lh"
    vec3f oc = vec3Sub(ray.origin, sphere.center);
#line 539 "/usr/include/lb/MathGraphic.lh"
    float a = vec3Dot(ray.direction, ray.direction);
#line 540 "/usr/include/lb/MathGraphic.lh"
    float b = (2.0*vec3Dot(oc, ray.direction));
#line 541 "/usr/include/lb/MathGraphic.lh"
    float c = (vec3Dot(oc, oc)-(sphere.radius*sphere.radius));
#line 542 "/usr/include/lb/MathGraphic.lh"
    float discriminant = ((b*b)-((4.0*a)*c));
#line 543 "/usr/include/lb/MathGraphic.lh"
    if (((discriminant<0.0))) {
#line 544 "/usr/include/lb/MathGraphic.lh"
    return result;
} else {
#line 546 "/usr/include/lb/MathGraphic.lh"
    float sqrtDisc = sqrt(discriminant);
#line 547 "/usr/include/lb/MathGraphic.lh"
    float t0 = ((((-b)-sqrtDisc))/((2.0*a)));
#line 548 "/usr/include/lb/MathGraphic.lh"
    float t1 = ((((-b)+sqrtDisc))/((2.0*a)));
#line 549 "/usr/include/lb/MathGraphic.lh"
    if (((t0>0.0))) {
#line 550 "/usr/include/lb/MathGraphic.lh"
    result.t = t0;
#line 551 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 552 "/usr/include/lb/MathGraphic.lh"
    return result;
} else if (((t1>0.0))) {
#line 554 "/usr/include/lb/MathGraphic.lh"
    result.t = t1;
#line 555 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 556 "/usr/include/lb/MathGraphic.lh"
    return result;
} else {
#line 558 "/usr/include/lb/MathGraphic.lh"
    return result;
}
}
}

#line 582 "/usr/include/lb/MathGraphic.lh"
vec3f reflect(vec3f v, vec3f n) {
#line 583 "/usr/include/lb/MathGraphic.lh"
    return vec3Sub(v, vec3Scale(n, (2.0*vec3Dot(v, n))));
}

#line 585 "/usr/include/lb/MathGraphic.lh"
vec3f refract(vec3f v, vec3f n, float eta) {
#line 586 "/usr/include/lb/MathGraphic.lh"
    float cosi = clamp(vec3Dot(v, n), (-1.0), 1.0);
#line 587 "/usr/include/lb/MathGraphic.lh"
    float etai = 1.0;
#line 588 "/usr/include/lb/MathGraphic.lh"
    float etat = eta;
#line 589 "/usr/include/lb/MathGraphic.lh"
    vec3f n_refract = n;
#line 590 "/usr/include/lb/MathGraphic.lh"
    if (((cosi<0.0))) {
#line 591 "/usr/include/lb/MathGraphic.lh"
    cosi = (-cosi);
} else {
#line 593 "/usr/include/lb/MathGraphic.lh"
    float temp_swap = etai;
#line 594 "/usr/include/lb/MathGraphic.lh"
    etai = etat;
#line 595 "/usr/include/lb/MathGraphic.lh"
    etat = temp_swap;
#line 596 "/usr/include/lb/MathGraphic.lh"
    n_refract = vec3Scale(n, (-1.0));
}
#line 598 "/usr/include/lb/MathGraphic.lh"
    float etaRatio = (etai/etat);
#line 599 "/usr/include/lb/MathGraphic.lh"
    float k = (1.0-((etaRatio*etaRatio)*((1.0-(cosi*cosi)))));
#line 600 "/usr/include/lb/MathGraphic.lh"
    if (((k<0.0))) {
#line 601 "/usr/include/lb/MathGraphic.lh"
    return v3(0.0, 0.0, 0.0);
} else {
#line 603 "/usr/include/lb/MathGraphic.lh"
    return vec3Add(vec3Scale(v, etaRatio), vec3Scale(n_refract, ((etaRatio*cosi)-sqrt(k))));
}
}

#line 606 "/usr/include/lb/MathGraphic.lh"
float fresnel(vec3f v, vec3f n, float eta) {
#line 607 "/usr/include/lb/MathGraphic.lh"
    float cosi = clamp(vec3Dot(v, n), (-1.0), 1.0);
#line 608 "/usr/include/lb/MathGraphic.lh"
    float etai = 1.0;
#line 609 "/usr/include/lb/MathGraphic.lh"
    float etat = eta;
#line 610 "/usr/include/lb/MathGraphic.lh"
    if (((cosi>0.0))) {
#line 611 "/usr/include/lb/MathGraphic.lh"
    float temp_swap = etai;
#line 612 "/usr/include/lb/MathGraphic.lh"
    etai = etat;
#line 613 "/usr/include/lb/MathGraphic.lh"
    etat = temp_swap;
}
#line 615 "/usr/include/lb/MathGraphic.lh"
    float sint = ((etai/etat)*sqrt(max(0.0, (1.0-(cosi*cosi)))));
#line 616 "/usr/include/lb/MathGraphic.lh"
    if (((sint>=1.0))) {
#line 617 "/usr/include/lb/MathGraphic.lh"
    return 1.0;
} else {
#line 619 "/usr/include/lb/MathGraphic.lh"
    float cost = sqrt(max(0.0, (1.0-(sint*sint))));
#line 620 "/usr/include/lb/MathGraphic.lh"
    cosi = fabsf(cosi);
#line 621 "/usr/include/lb/MathGraphic.lh"
    float Rs = (((((etat*cosi))-((etai*cost))))/((((etat*cosi))+((etai*cost)))));
#line 622 "/usr/include/lb/MathGraphic.lh"
    float Rp = (((((etai*cosi))-((etat*cost))))/((((etai*cosi))+((etat*cost)))));
#line 623 "/usr/include/lb/MathGraphic.lh"
    return ((((Rs*Rs)+(Rp*Rp)))/2.0);
}
}

#line 640 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayTriangle(rayf ray, vec3f v0, vec3f v1, vec3f v2) {
#line 641 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 642 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 643 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 644 "/usr/include/lb/MathGraphic.lh"
    vec3f edge1 = vec3Sub(v1, v0);
#line 645 "/usr/include/lb/MathGraphic.lh"
    vec3f edge2 = vec3Sub(v2, v0);
#line 646 "/usr/include/lb/MathGraphic.lh"
    vec3f h = vec3Cross(ray.direction, edge2);
#line 647 "/usr/include/lb/MathGraphic.lh"
    float a = vec3Dot(edge1, h);
#line 648 "/usr/include/lb/MathGraphic.lh"
    if (((fabsf(a)<0.000001))) {
#line 649 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 651 "/usr/include/lb/MathGraphic.lh"
    float f = (1.0/a);
#line 652 "/usr/include/lb/MathGraphic.lh"
    vec3f s = vec3Sub(ray.origin, v0);
#line 653 "/usr/include/lb/MathGraphic.lh"
    float u = (f*vec3Dot(s, h));
#line 654 "/usr/include/lb/MathGraphic.lh"
    if ((((u<0.0) || (u>1.0)))) {
#line 655 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 657 "/usr/include/lb/MathGraphic.lh"
    vec3f q = vec3Cross(s, edge1);
#line 658 "/usr/include/lb/MathGraphic.lh"
    float v = (f*vec3Dot(ray.direction, q));
#line 659 "/usr/include/lb/MathGraphic.lh"
    if ((((v<0.0) || ((u+v)>1.0)))) {
#line 660 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 662 "/usr/include/lb/MathGraphic.lh"
    float t_temp = (f*vec3Dot(edge2, q));
#line 663 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>0.000001))) {
#line 664 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 665 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 666 "/usr/include/lb/MathGraphic.lh"
    return result;
} else {
#line 668 "/usr/include/lb/MathGraphic.lh"
    return result;
}
}

#line 671 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayPlane(rayf ray, vec3f planeNormal, float planeDistance) {
#line 672 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 673 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 674 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 675 "/usr/include/lb/MathGraphic.lh"
    float denom = vec3Dot(planeNormal, ray.direction);
#line 676 "/usr/include/lb/MathGraphic.lh"
    if (((fabsf(denom)>0.000001))) {
#line 677 "/usr/include/lb/MathGraphic.lh"
    float t_temp = ((-((vec3Dot(planeNormal, ray.origin)+planeDistance)))/denom);
#line 678 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 679 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 680 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 681 "/usr/include/lb/MathGraphic.lh"
    return result;
}
}
#line 684 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 686 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayCapsule(rayf ray, vec3f p0, vec3f p1, float radius) {
#line 687 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 688 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 689 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 690 "/usr/include/lb/MathGraphic.lh"
    vec3f d = vec3Sub(p1, p0);
#line 691 "/usr/include/lb/MathGraphic.lh"
    vec3f m = vec3Sub(ray.origin, p0);
#line 692 "/usr/include/lb/MathGraphic.lh"
    vec3f n = ray.direction;
#line 693 "/usr/include/lb/MathGraphic.lh"
    float md = vec3Dot(m, d);
#line 694 "/usr/include/lb/MathGraphic.lh"
    float nd = vec3Dot(n, d);
#line 695 "/usr/include/lb/MathGraphic.lh"
    float dd = vec3Dot(d, d);
#line 696 "/usr/include/lb/MathGraphic.lh"
    if (((md<0.0))) {
#line 697 "/usr/include/lb/MathGraphic.lh"
    float t_temp = ((vec3Dot(m, m)-(2.0*md))+dd);
#line 698 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp<0.0))) {
#line 699 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 701 "/usr/include/lb/MathGraphic.lh"
    t_temp = (sqrt(t_temp)-md);
#line 702 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 703 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 704 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 705 "/usr/include/lb/MathGraphic.lh"
    return result;
}
} else if (((md>dd))) {
#line 708 "/usr/include/lb/MathGraphic.lh"
    float t_temp = ((vec3Dot(m, m)-(2.0*md))+dd);
#line 709 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp<0.0))) {
#line 710 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 712 "/usr/include/lb/MathGraphic.lh"
    t_temp = ((sqrt(t_temp)+md)-dd);
#line 713 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 714 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 715 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 716 "/usr/include/lb/MathGraphic.lh"
    return result;
}
} else {
#line 719 "/usr/include/lb/MathGraphic.lh"
    float t_temp = (vec3Dot(m, m)-((md*md)/dd));
#line 720 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp<0.0))) {
#line 721 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 723 "/usr/include/lb/MathGraphic.lh"
    t_temp = (sqrt(t_temp)-(((md/dd))*nd));
#line 724 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 725 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 726 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 727 "/usr/include/lb/MathGraphic.lh"
    return result;
}
}
#line 730 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 732 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayCylinder(rayf ray, vec3f p0, vec3f p1, float radius) {
#line 733 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 734 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 735 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 736 "/usr/include/lb/MathGraphic.lh"
    vec3f d = vec3Sub(p1, p0);
#line 737 "/usr/include/lb/MathGraphic.lh"
    vec3f m = vec3Sub(ray.origin, p0);
#line 738 "/usr/include/lb/MathGraphic.lh"
    vec3f n = ray.direction;
#line 739 "/usr/include/lb/MathGraphic.lh"
    float md = vec3Dot(m, d);
#line 740 "/usr/include/lb/MathGraphic.lh"
    float nd = vec3Dot(n, d);
#line 741 "/usr/include/lb/MathGraphic.lh"
    float dd = vec3Dot(d, d);
#line 742 "/usr/include/lb/MathGraphic.lh"
    float a = ((dd*vec3Dot(n, n))-(nd*nd));
#line 743 "/usr/include/lb/MathGraphic.lh"
    float k = (vec3Dot(m, m)-(radius*radius));
#line 744 "/usr/include/lb/MathGraphic.lh"
    float c = ((dd*k)-(md*md));
#line 745 "/usr/include/lb/MathGraphic.lh"
    if (((fabsf(a)<0.000001))) {
#line 746 "/usr/include/lb/MathGraphic.lh"
    if (((c>0.0))) {
#line 747 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 749 "/usr/include/lb/MathGraphic.lh"
    if (((md<0.0))) {
#line 750 "/usr/include/lb/MathGraphic.lh"
    float t_temp = ((-md)/nd);
#line 751 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 752 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 753 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 754 "/usr/include/lb/MathGraphic.lh"
    return result;
}
} else if (((md>dd))) {
#line 757 "/usr/include/lb/MathGraphic.lh"
    float t_temp = (((dd-md))/nd);
#line 758 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 759 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 760 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 761 "/usr/include/lb/MathGraphic.lh"
    return result;
}
} else {
#line 764 "/usr/include/lb/MathGraphic.lh"
    float t_temp = 0.0;
#line 765 "/usr/include/lb/MathGraphic.lh"
    if (((nd>0.0))) {
#line 766 "/usr/include/lb/MathGraphic.lh"
    t_temp = ((-md)/nd);
} else {
#line 768 "/usr/include/lb/MathGraphic.lh"
    t_temp = (((dd-md))/nd);
}
#line 770 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 771 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 772 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 773 "/usr/include/lb/MathGraphic.lh"
    return result;
}
}
#line 776 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 778 "/usr/include/lb/MathGraphic.lh"
    float b = (2.0*(((dd*vec3Dot(m, n))-(md*nd))));
#line 779 "/usr/include/lb/MathGraphic.lh"
    float discriminant = ((b*b)-((4.0*a)*c));
#line 780 "/usr/include/lb/MathGraphic.lh"
    if (((discriminant<0.0))) {
#line 781 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 783 "/usr/include/lb/MathGraphic.lh"
    float sqrtDisc = sqrt(discriminant);
#line 784 "/usr/include/lb/MathGraphic.lh"
    float t0 = ((((-b)-sqrtDisc))/((2.0*a)));
#line 785 "/usr/include/lb/MathGraphic.lh"
    float t1 = ((((-b)+sqrtDisc))/((2.0*a)));
#line 786 "/usr/include/lb/MathGraphic.lh"
    if (((t0>t1))) {
#line 787 "/usr/include/lb/MathGraphic.lh"
    float temp_swap = t0;
#line 788 "/usr/include/lb/MathGraphic.lh"
    t0 = t1;
#line 789 "/usr/include/lb/MathGraphic.lh"
    t1 = temp_swap;
}
#line 791 "/usr/include/lb/MathGraphic.lh"
    float y0 = (md+(t0*nd));
#line 792 "/usr/include/lb/MathGraphic.lh"
    float y1 = (md+(t1*nd));
#line 793 "/usr/include/lb/MathGraphic.lh"
    if (((y0<0.0))) {
#line 794 "/usr/include/lb/MathGraphic.lh"
    if (((y1<0.0))) {
#line 795 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 797 "/usr/include/lb/MathGraphic.lh"
    float t_temp = (t0+(((t1-t0))*((y0/((y0-y1))))));
#line 798 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 799 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 800 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 801 "/usr/include/lb/MathGraphic.lh"
    return result;
}
} else if (((y0>dd))) {
#line 804 "/usr/include/lb/MathGraphic.lh"
    if (((y1>dd))) {
#line 805 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 807 "/usr/include/lb/MathGraphic.lh"
    float t_temp = (t0+(((t1-t0))*((((y0-dd))/((y0-y1))))));
#line 808 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp>=0.0))) {
#line 809 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 810 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 811 "/usr/include/lb/MathGraphic.lh"
    return result;
}
} else {
#line 814 "/usr/include/lb/MathGraphic.lh"
    if (((t0>=0.0))) {
#line 815 "/usr/include/lb/MathGraphic.lh"
    result.t = t0;
#line 816 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 817 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 819 "/usr/include/lb/MathGraphic.lh"
    if (((t1>=0.0))) {
#line 820 "/usr/include/lb/MathGraphic.lh"
    result.t = t1;
#line 821 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 822 "/usr/include/lb/MathGraphic.lh"
    return result;
}
}
#line 825 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 827 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayCone(rayf ray, vec3f apex, vec3f axis, float angle) {
#line 828 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 829 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 830 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 831 "/usr/include/lb/MathGraphic.lh"
    float cosAngle = cos(angle);
#line 832 "/usr/include/lb/MathGraphic.lh"
    float cosAngleSq = (cosAngle*cosAngle);
#line 833 "/usr/include/lb/MathGraphic.lh"
    vec3f co = vec3Sub(ray.origin, apex);
#line 834 "/usr/include/lb/MathGraphic.lh"
    float vDotA = vec3Dot(ray.direction, axis);
#line 835 "/usr/include/lb/MathGraphic.lh"
    float coDotA = vec3Dot(co, axis);
#line 836 "/usr/include/lb/MathGraphic.lh"
    vec3f vPerp = vec3Sub(ray.direction, vec3Scale(axis, vDotA));
#line 837 "/usr/include/lb/MathGraphic.lh"
    vec3f coPerp = vec3Sub(co, vec3Scale(axis, coDotA));
#line 838 "/usr/include/lb/MathGraphic.lh"
    float a = ((cosAngleSq*vec3Dot(vPerp, vPerp))-(vDotA*vDotA));
#line 839 "/usr/include/lb/MathGraphic.lh"
    float b = (2.0*(((cosAngleSq*vec3Dot(vPerp, coPerp))-(vDotA*coDotA))));
#line 840 "/usr/include/lb/MathGraphic.lh"
    float c = ((cosAngleSq*vec3Dot(coPerp, coPerp))-(coDotA*coDotA));
#line 841 "/usr/include/lb/MathGraphic.lh"
    float discriminant = ((b*b)-((4.0*a)*c));
#line 842 "/usr/include/lb/MathGraphic.lh"
    if (((discriminant<0.0))) {
#line 843 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 845 "/usr/include/lb/MathGraphic.lh"
    float sqrtDisc = sqrt(discriminant);
#line 846 "/usr/include/lb/MathGraphic.lh"
    float t0 = ((((-b)-sqrtDisc))/((2.0*a)));
#line 847 "/usr/include/lb/MathGraphic.lh"
    float t1 = ((((-b)+sqrtDisc))/((2.0*a)));
#line 848 "/usr/include/lb/MathGraphic.lh"
    if (((t0>t1))) {
#line 849 "/usr/include/lb/MathGraphic.lh"
    float temp_swap = t0;
#line 850 "/usr/include/lb/MathGraphic.lh"
    t0 = t1;
#line 851 "/usr/include/lb/MathGraphic.lh"
    t1 = temp_swap;
}
#line 853 "/usr/include/lb/MathGraphic.lh"
    if (((t0>=0.0))) {
#line 854 "/usr/include/lb/MathGraphic.lh"
    result.t = t0;
#line 855 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 856 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 858 "/usr/include/lb/MathGraphic.lh"
    if (((t1>=0.0))) {
#line 859 "/usr/include/lb/MathGraphic.lh"
    result.t = t1;
#line 860 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 861 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 863 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 865 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayDisc(rayf ray, vec3f center, vec3f normal, float radius) {
#line 866 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 867 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 868 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 869 "/usr/include/lb/MathGraphic.lh"
    float denom = vec3Dot(normal, ray.direction);
#line 870 "/usr/include/lb/MathGraphic.lh"
    if (((fabsf(denom)<0.000001))) {
#line 871 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 873 "/usr/include/lb/MathGraphic.lh"
    float t_temp = (vec3Dot(vec3Sub(center, ray.origin), normal)/denom);
#line 874 "/usr/include/lb/MathGraphic.lh"
    if (((t_temp<0.0))) {
#line 875 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 877 "/usr/include/lb/MathGraphic.lh"
    vec3f p = vec3Add(ray.origin, vec3Scale(ray.direction, t_temp));
#line 878 "/usr/include/lb/MathGraphic.lh"
    if (((vec3Length(vec3Sub(p, center))>radius))) {
#line 879 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 881 "/usr/include/lb/MathGraphic.lh"
    result.t = t_temp;
#line 882 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 883 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 886 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayEllipsoid(rayf ray, vec3f center, vec3f radii) {
#line 887 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 888 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 889 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 890 "/usr/include/lb/MathGraphic.lh"
    vec3f m = vec3Sub(ray.origin, center);
#line 891 "/usr/include/lb/MathGraphic.lh"
    vec3f n = ray.direction;
#line 892 "/usr/include/lb/MathGraphic.lh"
    float a = (((((n.x*n.x))/((radii.x*radii.x)))+(((n.y*n.y))/((radii.y*radii.y))))+(((n.z*n.z))/((radii.z*radii.z))));
#line 893 "/usr/include/lb/MathGraphic.lh"
    float b = (2.0*((((((m.x*n.x))/((radii.x*radii.x)))+(((m.y*n.y))/((radii.y*radii.y))))+(((m.z*n.z))/((radii.z*radii.z))))));
#line 894 "/usr/include/lb/MathGraphic.lh"
    float c = ((((((m.x*m.x))/((radii.x*radii.x)))+(((m.y*m.y))/((radii.y*radii.y))))+(((m.z*m.z))/((radii.z*radii.z))))-1.0);
#line 895 "/usr/include/lb/MathGraphic.lh"
    float discriminant = ((b*b)-((4.0*a)*c));
#line 896 "/usr/include/lb/MathGraphic.lh"
    if (((discriminant<0.0))) {
#line 897 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 899 "/usr/include/lb/MathGraphic.lh"
    float sqrtDisc = sqrt(discriminant);
#line 900 "/usr/include/lb/MathGraphic.lh"
    float t0 = ((((-b)-sqrtDisc))/((2.0*a)));
#line 901 "/usr/include/lb/MathGraphic.lh"
    float t1 = ((((-b)+sqrtDisc))/((2.0*a)));
#line 902 "/usr/include/lb/MathGraphic.lh"
    if (((t0>t1))) {
#line 903 "/usr/include/lb/MathGraphic.lh"
    float temp_swap = t0;
#line 904 "/usr/include/lb/MathGraphic.lh"
    t0 = t1;
#line 905 "/usr/include/lb/MathGraphic.lh"
    t1 = temp_swap;
}
#line 907 "/usr/include/lb/MathGraphic.lh"
    if (((t0>=0.0))) {
#line 908 "/usr/include/lb/MathGraphic.lh"
    result.t = t0;
#line 909 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 910 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 912 "/usr/include/lb/MathGraphic.lh"
    if (((t1>=0.0))) {
#line 913 "/usr/include/lb/MathGraphic.lh"
    result.t = t1;
#line 914 "/usr/include/lb/MathGraphic.lh"
    result.hit = true;
#line 915 "/usr/include/lb/MathGraphic.lh"
    return result;
}
#line 917 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 919 "/usr/include/lb/MathGraphic.lh"
RayIntersectResult intersectRayTorus(rayf ray, vec3f center, vec3f normal, float majorRadius, float minorRadius) {
#line 920 "/usr/include/lb/MathGraphic.lh"
    RayIntersectResult result;
#line 921 "/usr/include/lb/MathGraphic.lh"
    result.hit = false;
#line 922 "/usr/include/lb/MathGraphic.lh"
    result.t = 0.0;
#line 923 "/usr/include/lb/MathGraphic.lh"
    vec3f m = vec3Sub(ray.origin, center);
#line 924 "/usr/include/lb/MathGraphic.lh"
    vec3f n = ray.direction;
#line 925 "/usr/include/lb/MathGraphic.lh"
    float a = vec3Dot(n, n);
#line 926 "/usr/include/lb/MathGraphic.lh"
    float b = (2.0*vec3Dot(m, n));
#line 927 "/usr/include/lb/MathGraphic.lh"
    float c = ((vec3Dot(m, m)+(majorRadius*majorRadius))-(minorRadius*minorRadius));
#line 928 "/usr/include/lb/MathGraphic.lh"
    float d = (vec3Dot(m, m)-(majorRadius*majorRadius));
#line 929 "/usr/include/lb/MathGraphic.lh"
    float coeffs[5];
#line 930 "/usr/include/lb/MathGraphic.lh"
    coeffs[(int)(0)] = (a*a);
#line 931 "/usr/include/lb/MathGraphic.lh"
    coeffs[(int)(1)] = ((2.0*a)*b);
#line 932 "/usr/include/lb/MathGraphic.lh"
    coeffs[(int)(2)] = ((b*b)+((2.0*a)*c));
#line 933 "/usr/include/lb/MathGraphic.lh"
    coeffs[(int)(3)] = ((2.0*b)*c);
#line 934 "/usr/include/lb/MathGraphic.lh"
    coeffs[(int)(4)] = ((c*c)-(((4.0*majorRadius)*majorRadius)*d));
#line 935 "/usr/include/lb/MathGraphic.lh"
    float roots[4];
#line 937 "/usr/include/lb/MathGraphic.lh"
    return result;
}

#line 955 "/usr/include/lb/MathGraphic.lh"
mat4f affineInverse(mat4f m) {
#line 956 "/usr/include/lb/MathGraphic.lh"
    mat4f inv;
#line 957 "/usr/include/lb/MathGraphic.lh"
    inv.x = v4(m.x.x, m.y.x, m.z.x, 0.0);
#line 958 "/usr/include/lb/MathGraphic.lh"
    inv.y = v4(m.x.y, m.y.y, m.z.y, 0.0);
#line 959 "/usr/include/lb/MathGraphic.lh"
    inv.z = v4(m.x.z, m.y.z, m.z.z, 0.0);
#line 960 "/usr/include/lb/MathGraphic.lh"
    inv.w = v4((-vec3Dot(v3(m.x.x, m.y.x, m.z.x), v3(m.w.x, m.w.y, m.w.z))), (-vec3Dot(v3(m.x.y, m.y.y, m.z.y), v3(m.w.x, m.w.y, m.w.z))), (-vec3Dot(v3(m.x.z, m.y.z, m.z.z), v3(m.w.x, m.w.y, m.w.z))), 1.0);
#line 961 "/usr/include/lb/MathGraphic.lh"
    return inv;
}

#line 963 "/usr/include/lb/MathGraphic.lh"
mat4f orthonormalInverse(mat4f m) {
#line 964 "/usr/include/lb/MathGraphic.lh"
    mat4f inv;
#line 965 "/usr/include/lb/MathGraphic.lh"
    inv.x = v4(m.x.x, m.y.x, m.z.x, 0.0);
#line 966 "/usr/include/lb/MathGraphic.lh"
    inv.y = v4(m.x.y, m.y.y, m.z.y, 0.0);
#line 967 "/usr/include/lb/MathGraphic.lh"
    inv.z = v4(m.x.z, m.y.z, m.z.z, 0.0);
#line 968 "/usr/include/lb/MathGraphic.lh"
    inv.w = v4((-vec3Dot(v3(m.x.x, m.y.x, m.z.x), v3(m.w.x, m.w.y, m.w.z))), (-vec3Dot(v3(m.x.y, m.y.y, m.z.y), v3(m.w.x, m.w.y, m.w.z))), (-vec3Dot(v3(m.x.z, m.y.z, m.z.z), v3(m.w.x, m.w.y, m.w.z))), 1.0);
#line 969 "/usr/include/lb/MathGraphic.lh"
    return inv;
}

#line 971 "/usr/include/lb/MathGraphic.lh"
mat4f compose(vec3f translation, quatf rotation, vec3f scale) {
#line 972 "/usr/include/lb/MathGraphic.lh"
    mat4f m;
#line 973 "/usr/include/lb/MathGraphic.lh"
    mat4f rot = fromQuat(rotation);
#line 974 "/usr/include/lb/MathGraphic.lh"
    m.x = v4((rot.x.x*scale.x), (rot.y.x*scale.y), (rot.z.x*scale.z), 0.0);
#line 975 "/usr/include/lb/MathGraphic.lh"
    m.y = v4((rot.x.y*scale.x), (rot.y.y*scale.y), (rot.z.y*scale.z), 0.0);
#line 976 "/usr/include/lb/MathGraphic.lh"
    m.z = v4((rot.x.z*scale.x), (rot.y.z*scale.y), (rot.z.z*scale.z), 0.0);
#line 977 "/usr/include/lb/MathGraphic.lh"
    m.w = v4(translation.x, translation.y, translation.z, 1.0);
#line 978 "/usr/include/lb/MathGraphic.lh"
    return m;
}

#line 980 "/usr/include/lb/MathGraphic.lh"
void decompose(mat4f m, vec3f* translation, quatf* rotation, vec3f* scale) {
#line 981 "/usr/include/lb/MathGraphic.lh"
    scale->x = vec3Length(v3(m.x.x, m.y.x, m.z.x));
#line 982 "/usr/include/lb/MathGraphic.lh"
    scale->y = vec3Length(v3(m.x.y, m.y.y, m.z.y));
#line 983 "/usr/include/lb/MathGraphic.lh"
    scale->z = vec3Length(v3(m.x.z, m.y.z, m.z.z));
#line 984 "/usr/include/lb/MathGraphic.lh"
    if (((scale->x>0.0))) {
#line 985 "/usr/include/lb/MathGraphic.lh"
    m.x.x /= scale->x;
#line 986 "/usr/include/lb/MathGraphic.lh"
    m.y.x /= scale->x;
#line 987 "/usr/include/lb/MathGraphic.lh"
    m.z.x /= scale->x;
}
#line 989 "/usr/include/lb/MathGraphic.lh"
    if (((scale->y>0.0))) {
#line 990 "/usr/include/lb/MathGraphic.lh"
    m.x.y /= scale->y;
#line 991 "/usr/include/lb/MathGraphic.lh"
    m.y.y /= scale->y;
#line 992 "/usr/include/lb/MathGraphic.lh"
    m.z.y /= scale->y;
}
#line 994 "/usr/include/lb/MathGraphic.lh"
    if (((scale->z>0.0))) {
#line 995 "/usr/include/lb/MathGraphic.lh"
    m.x.z /= scale->z;
#line 996 "/usr/include/lb/MathGraphic.lh"
    m.y.z /= scale->z;
#line 997 "/usr/include/lb/MathGraphic.lh"
    m.z.z /= scale->z;
}
#line 999 "/usr/include/lb/MathGraphic.lh"
    *rotation = fromMat4(m);
#line 1000 "/usr/include/lb/MathGraphic.lh"
    translation->x = m.w.x;
#line 1001 "/usr/include/lb/MathGraphic.lh"
    translation->y = m.w.y;
#line 1002 "/usr/include/lb/MathGraphic.lh"
    translation->z = m.w.z;
}

#line 1004 "/usr/include/lb/MathGraphic.lh"
mat4f fromTRS(vec3f translation, quatf rotation, vec3f scale) {
#line 1005 "/usr/include/lb/MathGraphic.lh"
    return compose(translation, rotation, scale);
}

#line 1007 "/usr/include/lb/MathGraphic.lh"
void toTRS(mat4f m, vec3f* translation, quatf* rotation, vec3f* scale) {
#line 1008 "/usr/include/lb/MathGraphic.lh"
    decompose(m, translation, rotation, scale);
}

#line 1010 "/usr/include/lb/MathGraphic.lh"
mat4f fromSRT(vec3f scale, quatf rotation, vec3f translation) {
#line 1011 "/usr/include/lb/MathGraphic.lh"
    return compose(translation, rotation, scale);
}

#line 1013 "/usr/include/lb/MathGraphic.lh"
void toSRT(mat4f m, vec3f* scale, quatf* rotation, vec3f* translation) {
#line 1014 "/usr/include/lb/MathGraphic.lh"
    decompose(m, translation, rotation, scale);
}

#line 1016 "/usr/include/lb/MathGraphic.lh"
mat4f fromRS(quatf rotation, vec3f scale) {
#line 1017 "/usr/include/lb/MathGraphic.lh"
    return compose(v3(0.0, 0.0, 0.0), rotation, scale);
}

#line 1019 "/usr/include/lb/MathGraphic.lh"
void toRS(mat4f m, quatf* rotation, vec3f* scale) {
#line 1020 "/usr/include/lb/MathGraphic.lh"
    vec3f x = v3(0.0, 0.0, 0.0);
#line 1021 "/usr/include/lb/MathGraphic.lh"
    decompose(m, &x, rotation, scale);
}

#line 1023 "/usr/include/lb/MathGraphic.lh"
mat4f fromSR(vec3f scale, quatf rotation) {
#line 1024 "/usr/include/lb/MathGraphic.lh"
    return compose(v3(0.0, 0.0, 0.0), rotation, scale);
}

#line 1026 "/usr/include/lb/MathGraphic.lh"
void toSR(mat4f m, vec3f* scale, quatf* rotation) {
#line 1027 "/usr/include/lb/MathGraphic.lh"
    vec3f x = v3(0.0, 0.0, 0.0);
#line 1028 "/usr/include/lb/MathGraphic.lh"
    decompose(m, &x, rotation, scale);
}

#line 4 "/usr/include/lb/Math.lh"
float fClamp(float min, float max, float x) {
#line 6 "/usr/include/lb/Math.lh"
    return ((x<min)?min:(((x>max)?max:x)));
}

#line 8 "/usr/include/lb/Math.lh"
float fLerp(float a, float b, float t) {
#line 9 "/usr/include/lb/Math.lh"
    return (a+(t*((b-a))));
}

#line 11 "/usr/include/lb/Math.lh"
float precisefLerp(float a, float b, float t) {
#line 12 "/usr/include/lb/Math.lh"
    return ((((1-t))*a)+(t*b));
}

#line 14 "/usr/include/lb/Math.lh"
int idsgn(double x) {
#line 15 "/usr/include/lb/Math.lh"
    return (((x>0))-((x<0)));
}

#line 17 "/usr/include/lb/Math.lh"
float fMap(float x, float in_min, float in_max, float out_min, float out_max) {
#line 18 "/usr/include/lb/Math.lh"
    return (((((x-in_min))*((out_max-out_min)))/((in_max-in_min)))+out_min);
}

#line 20 "/usr/include/lb/Math.lh"
float fSmoothStep(float edge0, float edge1, float x) {
#line 21 "/usr/include/lb/Math.lh"
    float t = (((x-edge0))/((edge1-edge0)));
#line 22 "/usr/include/lb/Math.lh"
    return (((t*t)*((3.0-(2.0*t)))));
}

#line 24 "/usr/include/lb/Math.lh"
float wrap(double x, double min, double max) {
#line 25 "/usr/include/lb/Math.lh"
    return (x-(((max-min))*floor((((x-min))/((max-min))))));
}

#line 27 "/usr/include/lb/Math.lh"
bool isClose(float a, float b, float epsilon) {
#line 28 "/usr/include/lb/Math.lh"
    return (fabsf((a-b))<=epsilon);
}

#line 30 "/usr/include/lb/Math.lh"
double sq(double x) {
#line 31 "/usr/include/lb/Math.lh"
    return (x*x);
}

#line 33 "/usr/include/lb/Math.lh"
float invfLerp(float a, float b, float val) {
#line 34 "/usr/include/lb/Math.lh"
    return (((val-a))/((b-a)));
}

#line 36 "/usr/include/lb/Math.lh"
float fStep(float edge, float x) {
#line 37 "/usr/include/lb/Math.lh"
    return ((x<edge)?0.0:1.0);
}

#line 39 "/usr/include/lb/Math.lh"
float fFract(float x) {
#line 40 "/usr/include/lb/Math.lh"
    return (x-floorf(x));
}

int main(int argc, char** argv) {
    #line 3 "/home/mebecool/Coding/lbreal/luaBase-Language/examples/typeinfer.lb"
vec4f x = v4(1, 2, 3, 5);
    #line 4 "/home/mebecool/Coding/lbreal/luaBase-Language/examples/typeinfer.lb"
printf("%s",TO_STR(sizeof(x))); printf("%s",TO_STR(", ")); printf("%s",TO_STR("vec4f")); printf("\n");
    return 0;
}