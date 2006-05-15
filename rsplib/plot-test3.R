source("plotter.R")

hideLegend          <- TRUE
colorMode           <- cmColor
legendPos           <- c(0,0.5)

generateOutput <- function(inFile, resultType, mainTitle="", summary=TRUE, yAxisTicks=c())
{
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
   else if(resultType =="QueueLength") {
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
                  xSeparatorsSet = xSeparatorsSet, xSeparatorsTitles = xSeparatorsTitles,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  type="h",
                  colorMode = colorMode, hideLegend = hideLegend,
                  legendPos = legendPos)
      }
   }
}


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




pdf("x.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)


#data <- loadResults("x.vec.bz2")
data <- loadResults("messung3a/pu-vectors.vec.bz2")


f <- (data$HandlingTime > 30)
bad <- subset(data$ObjectName, f)
cat("BAD: ", levels(factor(bad)), "\n")



handlingTimeStat(data, 1, 14)
handlingTimeStat(data, 16, 29)
handlingTimeStat(data, 31, 45)
handlingTimeStat(data, 46, 49)
handlingTimeStat(data, 51, 64)


xSeparatorsSet <- c(15, 30, 45, 50)
xSeparatorsTitles <- c("Failures\nin Asia",
                        "Backup\nCapacity",
                        "Reco-\nvery\nComp-\nleted",
                        "Normal\nOperation")
generateOutput("XXX/pu-vectors.vec.bz2", "HandlingSpeed", "Least Used Policy with Delay Penalty Factor",
               TRUE)
# generateOutput("XXX/pu-vectors.vec.bz2", "HandlingTime", "Least Used Policy with Delay Penalty Factor",
#                FALSE)

dev.off()
