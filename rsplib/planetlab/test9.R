distribute <- function(threads, width=1000, height=1000)
{
   yCount <- floor(sqrt(threads)) # floor(threads / sqrt(threads))
   yStep  <- round(height / yCount)

   cat("THREADS=",threads,"\n")

   cat("   yCount=",yCount,"\n")

   remaining <- threads
   number <- 0
   for(yPosition in seq(0, yCount - 1)) {
      if(remaining < 0) {
         stop("SHIT!");
      }

      if(yPosition < yCount - 1) {
         xCount <- min(remaining, yCount)
      }
      else {
         xCount <- remaining
      }

      if(xCount > 0) {
         xStep <- round(width / xCount)
         remaining <- remaining - xCount

         for(xPosition in seq(0, xCount - 1)) {
            cat("   ", number, ":", xPosition * xStep, " ; ", yPosition * yStep, "\n")
            number <- number + 1
         }
      }
   }

  if(number != threads) {
     stop("SHIT #2!");
  }
}


for(s in 1:512) {
   distribute(s)
}
