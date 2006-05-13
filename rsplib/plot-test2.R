source("plotter.R")

hideLegend          <- TRUE
colorMode           <- cmColor
legendPos           <- c(1,1)

generateOutput <- function(inFile, resultType, mainTitle="", summary=TRUE)
{

   data <- loadResults(inFile)

   xSet <- (data$CompleteTimeStamp / 60)
   xOffset <- -min(xSet)
   xSet <- xSet + xOffset
   xTitle <- "Time [Minutes]"

   hbarSet <- (data$QueuingTimeStamp / 60) + xOffset
   hbarMeanSteps <- 26

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

   if(summary) {
      plotstd3(mainTitle, xTitle, yTitle, zTitle, xSet, ySet,
               zSet, vSet, wSet, vTitle, wTitle,
               hbarSet = hbarSet, hbarMeanSteps = hbarMeanSteps,
               xSeparatorsSet = xSeparatorsSet, xSeparatorsTitles = xSeparatorsTitles,
               xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
               type="h",
               colorMode = colorMode, hideLegend = hideLegend,
               legendPos = legendPos)
   }
   else {
      for(z in levels(factor(zSet))) {
         cat(sep="", "z=", z, "\n")
         filter <- (zSet == z)
         xSubset <- subset(xSet, filter)
         ySubset <- subset(ySet, filter)
         zSubset <- subset(zSet, filter)
         vSubset <- subset(vSet, filter)
         wSubset <- subset(wSet, filter)
         hbarSubset <- subset(hbarSet, filter)
         plotstd3(paste(sep="", mainTitle, ", PU=", z),
                  xTitle, yTitle, zTitle, xSet, ySet,
                  zSubset, vSubset, wSubset, vTitle, wTitle,
                  hbarSet = hbarSubset, hbarMeanSteps = hbarMeanSteps,
                  xSeparatorsSet = xSeparatorsSet, xSeparatorsTitles = xSeparatorsTitles,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  type="h",
                  colorMode = colorMode, hideLegend = hideLegend,
                  legendPos = legendPos)
      }
   }
}



pdf("test2.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)

xSeparatorsSet <- c()
xSeparatorsTitles <- c()
# generateOutput("x.vec.bz2", "HandlingTime")
# generateOutput("x.vec.bz2", "HandlingSpeed")


xSeparatorsSet <- c(15, 30, 45, 50)
xSeparatorsTitles <- c("Failures\nin Asia",
                        "Backup\nCapacity",
                        "Reco-\nvery\nComp-\nleted",
                        "Normal\nOperation")
generateOutput("messung3/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy with Delay Penalty Factor")
generateOutput("messung4/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy")
generateOutput("messung3/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy with Delay Penalty Factor")
generateOutput("messung4/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy")

generateOutput("messung3/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy with Delay Penalty Factor", FALSE)
generateOutput("messung4/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy", FALSE)
generateOutput("messung3/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy with Delay Penalty Factor", FALSE)
generateOutput("messung4/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy", FALSE)

dev.off()
