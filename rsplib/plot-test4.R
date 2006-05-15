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
# generateOutput("messung3a/pu-vectors.vec.bz2", "HandlingTime", "3a Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,50,10))
# generateOutput("messung3b/pu-vectors.vec.bz2", "HandlingTime", "3b Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,50,10))
# generateOutput("messung3c/pu-vectors.vec.bz2", "HandlingTime", "3c Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,50,10))
# generateOutput("messung3d/pu-vectors.vec.bz2", "HandlingTime", "3d Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,50,10))
# generateOutput("messung3e/pu-vectors.vec.bz2", "HandlingTime", "3e Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,50,10))
# generateOutput("messung4a/pu-vectors.vec.bz2", "HandlingTime", "4a Least Used Policy ", TRUE,
#                seq(0,50,10))
# generateOutput("messung4b/pu-vectors.vec.bz2", "HandlingTime", "4b Least Used Policy ", TRUE,
#                seq(0,50,10))
# generateOutput("messung4c/pu-vectors.vec.bz2", "HandlingTime", "4c Least Used Policy ", TRUE,
#                seq(0,50,10))
# generateOutput("messung4d/pu-vectors.vec.bz2", "HandlingTime", "4d Least Used Policy ", TRUE,
#                seq(0,50,10))
# generateOutput("messung4e/pu-vectors.vec.bz2", "HandlingTime", "4e Least Used Policy ", TRUE,
#                seq(0,50,10))

# generateOutput("messung3a/pu-vectors.vec.bz2", "HandlingTime", "3a Least Used Policy with Delay Penalty Factor", TRUE,
#                seq(0,15,2))
generateOutput("messung3h/pu-vectors.vec.bz2", "HandlingTime", "3g Least Used Policy with Delay Penalty Factor", TRUE,
               seq(0,15,2))

# generateOutput("messung3a/pu-vectors.vec.bz2", "HandlingTime", "3a Least Used Policy with Delay Penalty Factor", FALSE,
#                seq(0,15,2))

# generateOutput("messung4d/pu-vectors.vec.bz2", "HandlingTime", "4d Least Used Policy ", TRUE,
#                seq(0,15,2))

dev.off()
