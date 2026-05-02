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

void addValue(int* thing, int val) {
    *thing = (((int)(*(thing)))+val);
}

float foo__int__float_(float x, float* y) {
    return (((x==0)?1:(x+*(y))));
}

int foo__int__int_(int x, int* y) {
    return (((x==0)?1:(x+*(y))));
}

int updateValue__int_(int* val) {
    if ((*(val)!=5)) {
    return 1;
}
    *val = 20;
    return 0;
}

int main(int argc, char** argv) {
    int y = 5;
    float xh = 2.1;
    float wda = foo__int__float_(1, &xh);
    addValue(&wda, 2);
    printf("%s",TO_STR(wda)); printf("\n");
    printf("%s",TO_STR("--")); printf("\n");
    int xz = foo__int__int_(1, &y);
    printf("%s",TO_STR(xz)); printf("\n");
    int vb = 5;
    int* ptr_to_vb = &vb;
    updateValue__int_(ptr_to_vb);
    printf("%s",TO_STR(vb)); printf("\n");
    return 0;
}