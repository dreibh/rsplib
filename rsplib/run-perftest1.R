# ###########################################################################
# Name:        test34
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")



# ====== Analyse results of a counter vector ================================
ACRT_Duration   <- 1
ACRT_Difference <- 2
ACRT_Normalized <- 3
analyseCounterResults <- function(data, minPreSkip, minPostSkip,
                                  segmentLength, segments,
                                  columnName,
                                  type)
{
   timeLow    <- min(data$RelTime)
   timeHigh   <- max(data$RelTime)
   resultsSet <- c()

   # ------ Cut off edges ---------------------------------------------------
   data <- subset(data, (data$RelTime >= timeLow + minPreSkip) &
                        (data$RelTime <= timeHigh - minPostSkip))
   if(timeHigh - (segmentLength * segments) < timeLow) {
      stop(paste(sep="", "ERROR: Data set is too short for ",
                 columnName , "; ", timeHigh - (segmentLength * segments), "s more required!"))
   }

   # ------ Fetch segments --------------------------------------------------
   for(i in 1:segments) {
      start <- timeLow + minPreSkip + ((i - 1) * segmentLength)
      end   <- timeLow + minPreSkip + (i * segmentLength)
      subData <- subset(data,
                        (data$RelTime >= start) &
                        (data$RelTime < end))

      xSet <- subData$RelTime
      ySet <- eval(parse(text=paste(sep="", "subData$", columnName)))

      duration   <- max(xSet) - min(xSet)
      difference <- max(ySet) - min(ySet)
      normalized <- difference / duration
      # cat(start,end, "  ->  ", duration, difference, normalized, "\n")

      if(type == ACRT_Duration) {
         resultsSet <- append(resultsSet, c(duration))
      }
      else if(type == ACRT_Difference) {
         resultsSet <- append(resultsSet, c(difference))
      }
      else {
         resultsSet <- append(resultsSet, c(normalized))
      }
   }
   return(resultsSet)
}


# ###########################################################################

testName <- "P01"

minPreSkip    <- 15
minPostSkip   <- 15
segmentLength <- 30
segments      <- 1

PEsSet           <- c(1,2,4,10,25,50,100,250)
PUsSet           <- c(1)
reregIntervalSet <- c(5000)
interHResTimeSet <- c(1000)
maxHResItemsSet  <- c(3)

# ###########################################################################


peColumn              <- c()
puColumn              <- c()
reregIntervalColumn   <- c()
interHResTimeColumn   <- c()
maxHResItemsColumn    <- c()

registrationRates     <- c()
reregistrationRates   <- c()
deregistrationRates   <- c()
handleResolutionRates <- c()
failureReportRates    <- c()
synchronizationRates  <- c()


# cat(readLines(pc <- pipe(paste(sep="", "rm -rf ", testName))))
# close(pc)
dir.create(testName,showWarnings=FALSE)


for(PEs in PEsSet) {
for(PUs in PUsSet) {
for(reregInterval in reregIntervalSet) {
for(interHResTime in interHResTimeSet) {
for(maxHResItems in maxHResItemsSet) { 
   # ------ Run performance test --------------------------------------------
   runPrefix <- paste(sep="", testName, "-", PEs, "-", PUs, "-", reregInterval, "-", interHResTime, "-", maxHResItems)
   cmdLine <- paste(sep="", "./perftest ",
                            testName, " ", runPrefix,
                            " ",
                            PEs, " ", PUs, " ", 
                            reregInterval, " ", interHResTime, " ", maxHResItems, " ",
                            minPreSkip + minPostSkip + (segments + 1) * segmentLength," ",
                            ">", testName, "/", runPrefix, ".log")
   cat(sep="", "Running ", cmdLine," ...\n")
   cat(readLines(pc <- pipe(cmdLine)))
   close(pc)

   # ------ Collect results -------------------------------------------------
   cat("Collecting results ...\n")
   data <- loadResults(paste(sep="", testName, "/", runPrefix, "/TestPR-1.data"))

   registrationResults <- analyseCounterResults(data, minPreSkip, minPostSkip, segmentLength, segments,
                                               "Registrations", ACRT_Normalized)

   registrationRates     <- append(registrationRates,
                                   registrationResults)
   reregistrationRates   <- append(reregistrationRates,
                                   analyseCounterResults(data, minPreSkip, minPostSkip, segmentLength, segments,
                                   "Reregistrations", ACRT_Normalized))
   deregistrationRates   <- append(deregistrationRates,
                                   analyseCounterResults(data, minPreSkip, minPostSkip, segmentLength, segments,
                                   "Deregistrations", ACRT_Normalized))
   handleResolutionRates <- append(handleResolutionRates,
                                   analyseCounterResults(data, minPreSkip, minPostSkip, segmentLength, segments,
                                   "HandleResolutions", ACRT_Normalized))
   failureReportRates    <- append(failureReportRates,
                                   analyseCounterResults(data, minPreSkip, minPostSkip, segmentLength, segments,
                                   "FailureReports", ACRT_Normalized))
   synchronizationRates  <- append(synchronizationRates,
                                   analyseCounterResults(data, minPreSkip, minPostSkip, segmentLength, segments,
                                   "Synchronizations", ACRT_Normalized))

   peColumn            <- append(peColumn, rep(PEs, length(registrationResults)))
   puColumn            <- append(puColumn, rep(PUs, length(registrationResults)))
   reregIntervalColumn <- append(reregIntervalColumn, rep(reregInterval, length(registrationResults)))
   interHResTimeColumn <- append(interHResTimeColumn, rep(interHResTime, length(registrationResults)))
   maxHResItemsColumn <- append(maxHResItemsColumn, rep(maxHResItems, length(registrationResults)))
}}}}}


cat("Writing results ...\n")
result <- data.frame(peColumn,
                     puColumn,
                     reregIntervalColumn,
                     interHResTimeColumn,
                     maxHResItemsColumn,

                     registrationRates,
                     reregistrationRates,
                     deregistrationRates,
                     handleResolutionRates,
                     failureReportRates,
                     synchronizationRates)
colnames(result) <- c("PEs", "PUs", "ReregInterval", "InterHResTime", "MaxHResItems",
                     "RegistrationRate", "ReregistrationRate", "DeregistrationRate",
                     "HandleResolutionRate", "FailureReportRate", "SynchronizationRate")
dir.create(paste(sep="", testName, "/Results"), showWarnings=FALSE)
resultsName <- paste(sep="", testName, "/Results/Summary.data")
write.table(result, resultsName)
cat(readLines(pc <- pipe(paste(sep="", "rm -f ", resultsName, ".bz2 ; bzip2 ", resultsName))))
close(pc)
