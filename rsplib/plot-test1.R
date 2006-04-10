source("plotter.R")


simulationDirectory <- "."
simulationName      <- "Influence of the PU:PE Ratio"
hideLegend          <- TRUE
colorMode           <- cmColor
pStart              <- 1
legendPos           <- c(1,1)



generateOutput <- function(inFile)
{

   data <- loadResults(inFile)

   xSet <- data$CompleteTimeStamp
   xOffset <- -min(xSet)
   xSet <- xSet + xOffset

   xTitle <- "Request Completion Time Stamp [s]"
   ySet <- data$HandlingSpeed
   yTitle <- "Request Handling Speed [Calculations/s]"
   #yTitle <- "Request Handling Time [s]"

   zSet <- data$ObjectName
   zTitle <- "PU"
   vSet <- c()
   vTitle <- ""
   wSet <- c()
   wTitle <- ""

   bSet <- c()
   bTitle <- ""
   aSet <- c()
   aTitle <- ""

   pSet <- c()
   pTitle <- ""


   xAxisTicks <- getIntegerTicks(seq(0, max(xSet) - 60))   # Set to c() for automatic setting
   yAxisTicks <- getIntegerTicks(ySet, count=10)   # Set to c() for automatic setting

   mainTitle <- paste(sep="", "Request Handling Performance ", inFile)

   hbarSet <- data$QueuingTimeStamp + xOffset
   plotstd6(mainTitle, pTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                        pSet, aSet, bSet, xSet, ySet, zSet,
                        vSet, wSet, vTitle, wTitle,
                        hbarSet = hbarSet,
                        xAxisTicks=xAxisTicks,yAxisTicks=yAxisTicks,
                        type="h",
                        colorMode=colorMode, hideLegend=hideLegend, pStart=pStart,
                        simulationName=simulationDirectory, legendPos=legendPos)
}



pdf("test1.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)

generateOutput("messung1/pu-vectors.vec.bz2")
generateOutput("messung2/pu-vectors.vec.bz2")
generateOutput("messung3/pu-vectors.vec.bz2")

dev.off()
