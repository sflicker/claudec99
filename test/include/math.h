#ifndef CLAUDEC99_MATH_H
#define CLAUDEC99_MATH_H

/* Basic math functions — double precision */
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double exp(double x);
double log(double x);
double log2(double x);
double log10(double x);
double pow(double x, double y);
double sqrt(double x);
double cbrt(double x);
double fabs(double x);
double floor(double x);
double ceil(double x);
double round(double x);
double trunc(double x);
double fmod(double x, double y);
double fmin(double x, double y);
double fmax(double x, double y);
double hypot(double x, double y);

/* Single-precision variants */
float sinf(float x);
float cosf(float x);
float tanf(float x);
float expf(float x);
float logf(float x);
float log2f(float x);
float log10f(float x);
float powf(float x, float y);
float sqrtf(float x);
float fabsf(float x);
float floorf(float x);
float ceilf(float x);
float roundf(float x);
float truncf(float x);
float fmodf(float x, float y);
float fminf(float x, float y);
float fmaxf(float x, float y);

/* Classification macros (integer-returning) */
int isnan(double x);
int isinf(double x);
int isfinite(double x);
int isnormal(double x);

/* Constants */
#define M_PI     3.14159265358979323846
#define M_E      2.71828182845904523536
#define M_SQRT2  1.41421356237309504880
#define M_LN2    0.69314718055994530942
#define M_LOG2E  1.44269504088896340736

#endif
