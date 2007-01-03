source("plotter.R")

ownOutput  <- FALSE
hideLegend <- TRUE
colorMode  <- cmColor
legendPos  <- c(1,1)


getTimeOffsetVector <- function(data)
{
   timeOffsetVector <- rep(-1e100, length(data$QueuingTimeStamp))
   for(m in levels(factor(data$Measurement))) {
      o <- -min(subset(data$QueuingTimeStamp, (data$Measurement == m)))
      timeOffsetVector <- replace(timeOffsetVector,
                                  which(data$Measurement == m),
                                  c(o))
   }
   return(timeOffsetVector)
}


getHandlingTimeVector <- function(data, timeOffsetVector, measurement, queuingStart, queuingEnd)
{
   correctedTime <- data$QueuingTimeStamp + timeOffsetVector
   queuingTime <- (correctedTime - min(correctedTime)) / 60
   f <- ((is.na(measurement)) | (data$Measurement == measurement)) &
        (queuingTime >= queuingStart) &
        (queuingTime <= queuingEnd)
   htSubset <- subset(data$HandlingTime, f)
   return(htSubset)
}


handlingTimeStat <- function(data, timeOffsetVector, measurement, queuingStart, queuingEnd)
{
   htSubset <- getHandlingTimeVector(data, timeOffsetVector, measurement, queuingStart, queuingEnd)
   htMin  <- min(htSubset)
   htMax  <- max(htSubset)
   htMean <- mean(htSubset)
   htTest <- t.test(htSubset)
   cat(sprintf("%2dm - %2dm:   mean=%1.5f +/- %1.5f   min=%1.5f max=%1.5f\n",
               queuingStart, queuingEnd,
               htMean, htMean - htTest$conf.int[1],
               htMin, htMax))
   return(htMean)
}


dumpAverageResults <- function(inFile, data, timeOffsetVector)
{
   cat("Average over all Measurements:\n")
   handlingTimeStat(data, timeOffsetVector, NA, 1, 14)
   handlingTimeStat(data, timeOffsetVector, NA, 16, 29)
   handlingTimeStat(data, timeOffsetVector, NA, 31, 45)
   handlingTimeStat(data, timeOffsetVector, NA, 46, 49)
   handlingTimeStat(data, timeOffsetVector, NA, 51, 64)

   mainTitle <- ""
   xTitle <- "Measurement"
   xSet <- c()
   yTitle <- "Handling Time [s]"
   ySet <- c()

   zTitle <- "System State"
   zSet <- c()
   vTitle <- ""
   vSet <- c()
   wTitle <- ""
   wSet <- c()

   for(m in levels(factor(data$Measurement))) {
      cat(sep="", "Measurement ", m, " (", inFile, "):\n")
      handlingTimeStat(data, timeOffsetVector, m, 1, 14)
      handlingTimeStat(data, timeOffsetVector, m, 16, 29)
      handlingTimeStat(data, timeOffsetVector, m, 31, 45)
      handlingTimeStat(data, timeOffsetVector, m, 46, 49)
      handlingTimeStat(data, timeOffsetVector, m, 51, 64)

      htVector <- c(mean(getHandlingTimeVector(data, timeOffsetVector, m, 1, 14)))
      xSet <- append(xSet, rep(m, length(htVector)))
      ySet <- append(ySet, htVector)
      zSet <- append(zSet, rep("1 - Normal Operation I", length(htVector)))

      htVector <- c(mean(getHandlingTimeVector(data, timeOffsetVector, m, 16, 29)))
      xSet <- append(xSet, rep(m, length(htVector)))
      ySet <- append(ySet, htVector)
      zSet <- append(zSet, rep("2 - Failure in Asia", length(htVector)))

      htVector <- c(mean(getHandlingTimeVector(data, timeOffsetVector, m, 31, 45)))
      xSet <- append(xSet, rep(m, length(htVector)))
      ySet <- append(ySet, htVector)
      zSet <- append(zSet, rep("3 - Backup Capacity", length(htVector)))

      htVector <- c(mean(getHandlingTimeVector(data, timeOffsetVector, m, 46, 49)))
      xSet <- append(xSet, rep(m, length(htVector)))
      ySet <- append(ySet, htVector)
      zSet <- append(zSet, rep("4 - Recovery Complete", length(htVector)))

      htVector <- c(mean(getHandlingTimeVector(data, timeOffsetVector, m, 51, 64)))
      xSet <- append(xSet, rep(m, length(htVector)))
      ySet <- append(ySet, htVector)
      zSet <- append(zSet, rep("5 - Normal Operation II", length(htVector)))
   }

   xSet <- factor(xSet)

   xAxisTicks <- c()
   yAxisTicks <- seq(1,10,1)
   legendPos  <- c(1,1)
   hideLegend <- FALSE

   plotstd3(mainTitle,
            xTitle, yTitle, zTitle, xSet, ySet,
            zSet, vSet, wSet, vTitle, wTitle,
            xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
            type="l",
            colorMode = colorMode, hideLegend = hideLegend,
            legendPos = legendPos)
}


dumpJobsResults <- function(inFile, data, timeOffsetVector,
                            resultType, mainTitle="", summary=TRUE, xAxisTicks=c(), yAxisTicks=c())
{
   zSet <- data$Measurement
   zTitle <- "Measurement"
   vSet <- c()   # data$ObjectName
   vTitle <- ""
   wSet <- c()
   wTitle <- ""

   xSet <- (data$CompleteTimeStamp + timeOffsetVector) / 60
   xTitle <- "Time [Minutes]"
   xSeparatorsSet <- c(15, 30, 45, 50)
   xSeparatorsTitles <- c("Failures\nin Asia",
                           "Backup\nCapacity",
                           "Reco-\nvery\nComp-\nleted",
                           "Normal\nOperation")

   hbarSet <- (data$QueuingTimeStamp + timeOffsetVector) / 60
   hbarMeanSteps <- 39

   aggregator <- hbarDefaultAggregator
   if(resultType == "HandlingTime") {
      ySet <- data$HandlingTime
      yTitle <- "Average Request Handling Time [s]"
   }
   else if(resultType == "HandlingSpeed") {
      ySet <- data$HandlingSpeed
      yTitle <- "Average Request Handling Speed [Calculations/s]"
      aggregator <- hbarHandlingSpeedAggregator
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
      checkSets(data, xSet, ySet, zSet, vSet, wSet)
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
         checkSets(data, xSet, ySet, zSet, vSet, wSet)
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



cat("Loading data ...\n")
inFileA <- "DisasterScenario1-A.data.bz2"
inFileB <- "DisasterScenario1-B.data.bz2"
dataA <- loadResults(inFileA)
dataB <- loadResults(inFileB)
timeOffsetVectorA <- getTimeOffsetVector(dataA)
timeOffsetVectorB <- getTimeOffsetVector(dataB)
cat("Loading data completed!\n")


if(!ownOutput) { pdf("test6.pdf", width=12.5, height=7.5, onefile=TRUE, family="Helvetica", pointsize=14) }

if(ownOutput) { pdf("test6-leastuseddpf.pdf", width=12.5, height=7.5, onefile=TRUE, family="Helvetica", pointsize=14) }
dumpAverageResults(inFileA, dataA, timeOffsetVectorA)
if(ownOutput) { dev.off() }
if(ownOutput) { pdf("test6-leastuseddpf-info.pdf", width=12.5, height=7.5, onefile=TRUE, family="Helvetica", pointsize=14) }
dumpJobsResults(inFileA, dataA, timeOffsetVectorA,
                "HandlingSpeed", "", TRUE,
                seq(0,65,5), seq(0,1000000,200000))
if(ownOutput) { dev.off() }


if(ownOutput) { pdf("test6-leastused.pdf", width=12.5, height=7.5, onefile=TRUE, family="Helvetica", pointsize=14) }
dumpAverageResults(inFileB, dataB, timeOffsetVectorB)
if(ownOutput) { dev.off() }
if(ownOutput) { pdf("test6-leastused-info.pdf", width=12.5, height=7.5, onefile=TRUE, family="Helvetica", pointsize=14) }
dumpJobsResults(inFileB, dataB, timeOffsetVectorB,
                "HandlingSpeed", "", TRUE,
                seq(0,65,5), seq(0,1000000,200000))
if(ownOutput) { dev.off() }

if(!ownOutput) { dev.off() }
