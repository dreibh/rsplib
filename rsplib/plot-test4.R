source("plotter.R")

hideLegend          <- TRUE
colorMode           <- cmColor
legendPos           <- c(1,1)

handlingTimeStat <- function(data, start, end)
{
   queuingTime <- (data$QueuingTimeStamp - min(data$QueuingTimeStamp)) / 60
   f <- (queuingTime >= start) &
        (queuingTime <= end)
   htSubset <- subset(data$HandlingTime, f)
   htMin  <- min(htSubset)
   htMax  <- max(htSubset)
   htMean <- mean(htSubset)
   htTest <- t.test(htSubset)
   cat(sep="", start, "-", end, ":   mean=", htMean,
       " +/- ", htMean - htTest$conf.int[1], "   ",
       "min=", htMin, " max=", htMax,
       "\n")
}

generateOutput <- function(inFile, resultType, mainTitle="", summary=TRUE, yAxisTicks=c())
{
   data <- loadResults(inFile)

   cat("-----",inFile,"-----\n")
   handlingTimeStat(data, 1, 14)
   handlingTimeStat(data, 16, 29)
   handlingTimeStat(data, 31, 45)
   handlingTimeStat(data, 46, 49)
   handlingTimeStat(data, 51, 64)

   xSet <- (data$CompleteTimeStamp / 60)
   xOffset <- -min(xSet)
   xSet <- xSet + xOffset
   xTitle <- "Time [Minutes]"

   hbarSet <- (data$QueuingTimeStamp / 60) + xOffset
   hbarMeanSteps <- 52

   aggregator <- hbarDefaultAggregator
   if(resultType == "HandlingTime") {
      ySet <- data$HandlingTime
      yTitle <- "Average Request Handling Time [s]"
   }
   else if(resultType == "HandlingSpeed") {
      ySet <- data$HandlingSpeed
      aggregator <- hbarHandlingSpeedAggregator
      yTitle <- "Average Request Handling Speed [Calculations/s]"
   }
   else if(resultType == "QueueLength") {
      ySet <- data$QueueLength
      yTitle <- "Average Queue Length [1]"
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
   if(length(yAxisTicks) < 2) {
      yAxisTicks <- getIntegerTicks(ySet, count=10)   # Set to c() for automatic setting
   }

   if(summary) {
      plotstd3(mainTitle, xTitle, yTitle, zTitle, xSet, ySet,
               zSet, vSet, wSet, vTitle, wTitle,
               hbarSet = hbarSet, hbarMeanSteps = hbarMeanSteps,
               hbarAggregator = aggregator,
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
                  xTitle, yTitle, zTitle,
                  xSubset, ySubset,
                  zSubset, vSubset, wSubset, vTitle, wTitle,
                  hbarSet = hbarSubset, hbarMeanSteps = hbarMeanSteps,
                  hbarAggregator = aggregator,
                  xSeparatorsSet = xSeparatorsSet, xSeparatorsTitles = xSeparatorsTitles,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  type="h",
                  colorMode = colorMode, hideLegend = hideLegend,
                  legendPos = legendPos)
      }
   }
}



pdf("test4.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)

# xSeparatorsSet <- c()
# xSeparatorsTitles <- c()
# generateOutput("messung1/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,50,10))
# generateOutput("messung2/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy", TRUE,
#                seq(0,50,10))
# generateOutput("messung1/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy with Delay Penalty Factor")
# generateOutput("messung2/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy")


xSeparatorsSet <- c(15, 30, 45, 50)
xSeparatorsTitles <- c("Failures\nin Asia",
                        "Backup\nCapacity",
                        "Reco-\nvery\nComp-\nleted",
                        "Normal\nOperation")
generateOutput("messung3l/pu-vectors.vec.bz2", "HandlingTime", "3l Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3m/pu-vectors.vec.bz2", "HandlingTime", "3m Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3n/pu-vectors.vec.bz2", "HandlingTime", "3n Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3o/pu-vectors.vec.bz2", "HandlingTime", "3o Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3p/pu-vectors.vec.bz2", "HandlingTime", "3p Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3q/pu-vectors.vec.bz2", "HandlingTime", "3q Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3r/pu-vectors.vec.bz2", "HandlingTime", "3r Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3s/pu-vectors.vec.bz2", "HandlingTime", "3s Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung3t/pu-vectors.vec.bz2", "HandlingTime", "3t Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))

generateOutput("messung4f/pu-vectors.vec.bz2", "HandlingTime", "4f Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung4g/pu-vectors.vec.bz2", "HandlingTime", "4g Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung4h/pu-vectors.vec.bz2", "HandlingTime", "4h Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung4i/pu-vectors.vec.bz2", "HandlingTime", "4i Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung4j/pu-vectors.vec.bz2", "HandlingTime", "4j Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))
generateOutput("messung4k/pu-vectors.vec.bz2", "HandlingTime", "4k Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,50,10))

dev.off()
