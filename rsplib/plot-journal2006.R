source("plotter.R")

ownOutput  <- FALSE
colorMode  <- cmColor
hideLegend <- FALSE
legendPos  <- c(1,1)

xAxisTicks <- seq(0,4,0.5)
yAxisTicks <- seq(0,0.6,0.1)
frameColor <- "gold4"


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


getHandlingTimeVector <- function(data, timeOffsetVector, queuingStart, queuingEnd)
{
   correctedTime <- data$QueuingTimeStamp + timeOffsetVector
   queuingTime <- (correctedTime - min(correctedTime)) / 60
   f <- (queuingTime >= queuingStart) &
        (queuingTime <= queuingEnd)
   return(subset(data$HandlingTime, f))
}


getPolicyVector <- function(data, timeOffsetVector, queuingStart, queuingEnd)
{
   correctedTime <- data$QueuingTimeStamp + timeOffsetVector
   queuingTime <- (correctedTime - min(correctedTime)) / 60
   f <- (queuingTime >= queuingStart) &
        (queuingTime <= queuingEnd)
   return(subset(data$Policy, f))
}


getMeasurementVector <- function(data, timeOffsetVector, queuingStart, queuingEnd)
{
   correctedTime <- data$QueuingTimeStamp + timeOffsetVector
   queuingTime <- (correctedTime - min(correctedTime)) / 60
   f <- (queuingTime >= queuingStart) &
        (queuingTime <= queuingEnd)
   return(subset(data$Measurement, f))
}


createHistogram <- function(pdfName, data, timeOffsetVector, queuingStart, queuingEnd, title)
{
   pdf(pdfName, width=10, height=7.5, onefile=TRUE, family="Helvetica", pointsize=18)

   policySubset <- getPolicyVector(data, timeOffsetVector, queuingStart, queuingEnd)
   measurementSubset <- getMeasurementVector(data, timeOffsetVector, queuingStart, queuingEnd)
   handlingTimeSubset <- getHandlingTimeVector(data, timeOffsetVector, queuingStart, queuingEnd)
   handlingTimeMin  <- min(handlingTimeSubset)
   handlingTimeMax  <- max(handlingTimeSubset)
   handlingTimeMean <- mean(handlingTimeSubset)
   handlingTimeTest <- t.test(handlingTimeSubset)
   cat(title,"\n")
   cat(sprintf("%2dm - %2dm:   mean=%1.5f +/- %1.5f   min=%1.5f max=%1.5f\n",
               queuingStart, queuingEnd,
               handlingTimeMean, handlingTimeMean - handlingTimeTest$conf.int[1],
               handlingTimeMin, handlingTimeMax))

   yTitle <- "Request Density [1]"

   xSet <- handlingTimeSubset
   xTitle <- "Request Handling Time Interval [s]"

   zSet <- policySubset
   zTitle <- "Pool Policy"

   cSet <- measurementSubset

   plothist(title, xTitle, yTitle, zTitle, xSet, ySet, zSet, cSet,
            freq=FALSE,
            xAxisTicks=xAxisTicks, yAxisTicks=yAxisTicks,
            frameColor=frameColor,
            colorMode=colorMode,
            hideLegend=hideLegend,
            showMinMax=TRUE,
            showConfidence=TRUE,
            confidence=0.95)
   dev.off()
}


cat("Loading data ...\n")
data <- loadResults("journal2006.data.bz2")
timeOffsetVector <- getTimeOffsetVector(data)
cat("Loading data completed!\n")


createHistogram("journal2006-normalOperationI.pdf", data, timeOffsetVector, 1, 14, "Normal Operation I")
createHistogram("journal2006-failureInAsia.pdf", data, timeOffsetVector, 16, 29, "Failure in Asia")
createHistogram("journal2006-addedBackupCapacity.pdf", data, timeOffsetVector, 31, 45, "Added Backup Capacity")
createHistogram("journal2006-failureResolved.pdf", data, timeOffsetVector, 46, 49, "Failure Resolved")
createHistogram("journal2006-normalOperationII.pdf", data, timeOffsetVector, 51, 64, "Normal Operation II")
