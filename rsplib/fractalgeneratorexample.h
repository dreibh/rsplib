#ifndef FRACTALGENERATOREXAMPLE_H
#define FRACTALGENERATOREXAMPLE_H


#define FGPA_MANDELBROT 1
#define FGPA_MANDELBROTN 2

struct FractalGeneratorParameter
{
   uint32_t Width;
   uint32_t Height;
   uint32_t MaxIterations;
   uint32_t AlgorithmID;
   double   C1Real;            /* How to handle Byte-Order??? */
   double   C1Imag;
   double   C2Real;
   double   C2Imag;
   double   N;
};


#define FGD_MAX_POINTS 1024

struct FractalGeneratorData
{
   uint32_t StartX;
   uint32_t StartY;
   uint32_t Points;
   uint32_t Buffer[FGD_MAX_POINTS];
};

inline size_t getFractalGeneratorDataSize(const size_t points)
{
   struct FractalGeneratorData fgd;
   return(((long)&fgd.Buffer - (long)&fgd) + sizeof(uint32_t) * points);
}



#define FG_COOKIE_ID "<FG-000>"

struct FractalGeneratorCookie
{
   char                      ID[8];
   FractalGeneratorParameter Parameter;
   uint32_t                  CurrentX;
   uint32_t                  CurrentY;
};


struct FractalGeneratorStatus {
   struct FractalGeneratorParameter Parameter;
   uint32_t                         CurrentX;
   uint32_t                         CurrentY;
};


#endif
