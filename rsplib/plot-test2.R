source("plotter.R")

hideLegend          <- TRUE
colorMode           <- cmColor
legendPos           <- c(1,1)

xSeparatorsSet <- c(15, 30, 45, 50)
xSeparatorsTitles <- c("Failures\nin Asia",
                        "Backup\nCapacity",
                        "Reco-\nvery\nComp-\nleted",
                        "Normal\nOperation")


generateOutput <- function(inFile, resultType, mainTitle="")
{

   data <- loadResults(inFile)

   xSet <- (data$CompleteTimeStamp / 60)
   xOffset <- -min(xSet)
   xSet <- xSet + xOffset
   xTitle <- "Time [Minutes]"

   hbarSet <- (data$QueuingTimeStamp / 60) + xOffset
   hbarMeanSteps <- 20

   if(resultType =="HandlingTime") {
      ySet <- data$HandlingTime
      yTitle <- "Average Request Handling Time [s]"
   }
   else if(resultType =="HandlingSpeed") {
      ySet <- data$HandlingSpeed
      yTitle <- "Average Request Handling Speed [Calculations/s]"
   }
   else {
      stop("Bad result type!")
   }

   zSet <- data$ObjectName
   zTitle <- ""
   vSet <- c()
   vTitle <- ""
   wSet <- c()
   wTitle <- ""

   xAxisTicks <- getUsefulTicks(xSet)   # getIntegerTicks(seq(0, max(xSet) - 60))   # Set to c() for automatic setting
   yAxisTicks <- getIntegerTicks(ySet, count=10)   # Set to c() for automatic setting

   plotstd3(mainTitle, xTitle, yTitle, zTitle, xSet, ySet,
            zSet, vSet, wSet, vTitle, wTitle,
            hbarSet = hbarSet, hbarMeanSteps = hbarMeanSteps,
            xSeparatorsSet = xSeparatorsSet, xSeparatorsTitles = xSeparatorsTitles,
            xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
            type="h",
            colorMode = colorMode, hideLegend = hideLegend,
            legendPos = legendPos)
}



pdf("test2.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)

# generateOutput("x.vec.bz2", "HandlingTime")
# generateOutput("x.vec.bz2", "HandlingSpeed")

generateOutput("messung3/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy with Delay Penalty Factor")
generateOutput("messung3/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy with Delay Penalty Factor")

generateOutput("messung4/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy")
generateOutput("messung4/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy")

dev.off()
