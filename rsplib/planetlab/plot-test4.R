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

generateOutput <- function(inFile, resultType, mainTitle="", summary=TRUE, xAxisTicks=c(), yAxisTicks=c())
{
   data <- loadResults(inFile)

   cat("-----",inFile,"-----\n")
   handlingTimeStat(data, 1, 14)
   handlingTimeStat(data, 16, 29)
   handlingTimeStat(data, 31, 45)
   handlingTimeStat(data, 46, 49)
   handlingTimeStat(data, 51, 64)


   zSet <- data$Measurement
   zTitle <- "Measurement"
   vSet <- c()   # data$ObjectName
   vTitle <- ""
   wSet <- c()
   wTitle <- ""


   xSet <- (data$CompleteTimeStamp / 60)
   xOffset <- rep(0, length(xSet))
   for(z in levels(factor(zSet))) {
      o <- -min(subset(xSet, (zSet == z)))
      # cat(z, o, " ...\n")
      for(i in seq(1, length(xSet))) {
         if(zSet[i] == z) {
            xOffset[i] <- o
         }
      }
   }

   xSet <- xSet + xOffset
   xTitle <- "Time [Minutes]"


   hbarSet <- (data$QueuingTimeStamp / 60) + xOffset
   hbarMeanSteps <- 39

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

   if(length(xAxisTicks) < 2) {
      xAxisTicks <- getUsefulTicks(xSet)
   }
   if(length(yAxisTicks) < 2) {
      yAxisTicks <- getIntegerTicks(ySet, count=10)
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



xSeparatorsSet <- c(15, 30, 45, 50)
xSeparatorsTitles <- c("Failures\nin Asia",
                        "Backup\nCapacity",
                        "Reco-\nvery\nComp-\nleted",
                        "Normal\nOperation")

scenario <- "DisasterScenario1"

pdf("test4.pdf", width=12.5, height=7.5, onefile=TRUE, family="Helvetica", pointsize=14)
generateOutput(paste(sep="", scenario, "-A.data.bz2"),
               "HandlingSpeed", "", TRUE,
               seq(0,65,5), seq(0,1000000,200000))
generateOutput(paste(sep="", scenario, "-B.data.bz2"),
               "HandlingSpeed", "", TRUE,
               seq(0,65,5), seq(0,1000000,200000))
dev.off()
