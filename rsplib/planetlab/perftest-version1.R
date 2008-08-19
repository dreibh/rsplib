# ###########################################################################
# Name:        perftest-version1.R
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")


# ====== Perform handlespace management performance measurement =============
performMeasurement <- function()
{
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

   runtimeDiffs          <- c()
   userTimeDiffs         <- c()
   systemTimeDiffs       <- c()


   dir.create(testName,showWarnings=FALSE)


   for(PEs in PEsSet) {
   for(puToPERatio in puToPERatioSet) {
   for(PUs in PUsSet) {
   if((length(PUsSet) == 1) && (PUs < 0)) {
      PUs <- PEs * puToPERatio
   }
   for(reregInterval in reregIntervalSet) {
   for(interHResTime in interHResTimeSet) {
   for(maxHResItems in maxHResItemsSet) {

      # ------ Run performance test --------------------------------------------
      duration <- minPreSkip + minPostSkip + (segments + 1) * segmentLength
      runPrefix <- paste(sep="", testName, "-", PEs, "-", PUs, "-", reregInterval, "-", interHResTime, "-", maxHResItems, "-", duration)
      cmdLine <- paste(sep="", "./perftest ",
                              testName, " ", runPrefix,
                              " ",
                              PEs, " ", PUs, " ", 
                              reregInterval, " ", interHResTime, " ", maxHResItems, " ",
                              duration, " ",
                              ">", testName, "/", runPrefix, ".log")
      cat(sep="", "Running ", cmdLine," ...\n")
      cat(readLines(pc <- pipe(cmdLine)))
      close(pc)

      # ------ Collect results -------------------------------------------------
      cat("Collecting results ...\n")
      data <- loadResults(paste(sep="", testName, "/", runPrefix, "/TestPR-1.data"))
      rtdata <- read.table(paste(sep="", testName, "/", runPrefix, "/runtimes.data"))
      lowerLimit  <- minPreSkip + (mean(rtdata$MeasurementStart) - mean(rtdata$Startup))
      upperLimit <- (mean(rtdata$MeasurementEnd) - mean(rtdata$Startup)) - minPostSkip

      cat("=> Startup:            0.0\n")
      cat("   Measurement Start: ", mean(rtdata$MeasurementStart) - mean(rtdata$Startup), "\n")
      cat("   Measurement End:   ", mean(rtdata$MeasurementEnd)   - mean(rtdata$Startup), "\n")
      cat("   Shutdown:          ", mean(rtdata$Shutdown)         - mean(rtdata$Startup), "\n")
      cat("=> ", duration,lowerLimit,upperLimit,"\n")

      registrationResults <- analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                                                   "Registrations", ACRT_Normalized)

      registrationRates     <- append(registrationRates,
                                    registrationResults)
      reregistrationRates   <- append(reregistrationRates,
                                    analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                                    "Reregistrations", ACRT_Normalized))
      deregistrationRates   <- append(deregistrationRates,
                                    analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                                    "Deregistrations", ACRT_Normalized))
      handleResolutionRates <- append(handleResolutionRates,
                                    analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                                    "HandleResolutions", ACRT_Normalized))
      failureReportRates    <- append(failureReportRates,
                                    analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                                    "FailureReports", ACRT_Normalized))
      synchronizationRates  <- append(synchronizationRates,
                                    analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                                    "Synchronizations", ACRT_Normalized))

      runtimeDiffs <- append(runtimeDiffs,
                           analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                           "Runtime", ACRT_Difference))
      userTimeDiffs <- append(userTimeDiffs,
                           analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                           "UserTime", ACRT_Difference))
      systemTimeDiffs <- append(systemTimeDiffs,
                           analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                           "SystemTime", ACRT_Difference))

      peColumn            <- append(peColumn, rep(PEs, length(registrationResults)))
      puColumn            <- append(puColumn, rep(PUs, length(registrationResults)))
      reregIntervalColumn <- append(reregIntervalColumn, rep(reregInterval, length(registrationResults)))
      interHResTimeColumn <- append(interHResTimeColumn, rep(interHResTime, length(registrationResults)))
      maxHResItemsColumn  <- append(maxHResItemsColumn,  rep(maxHResItems, length(registrationResults)))
   }}}}}}


   cat("Writing results ...\n")
   result <- data.frame(peColumn,
                        puColumn,
                        reregIntervalColumn,
                        interHResTimeColumn,
                        maxHResItemsColumn,

                        runtimeDiffs,
                        userTimeDiffs,
                        systemTimeDiffs,

                        registrationRates,
                        reregistrationRates,
                        deregistrationRates,
                        handleResolutionRates,
                        failureReportRates,
                        synchronizationRates)
   colnames(result) <- c("PEs", "PUs", "ReregInterval", "InterHResTime", "MaxHResItems",
                        "Runtime", "UserTime", "SystemTime",
                        "RegistrationRate", "ReregistrationRate", "DeregistrationRate",
                        "HandleResolutionRate", "FailureReportRate", "SynchronizationRate")
   dir.create(paste(sep="", testName, "/Results"), showWarnings=FALSE)
   resultsName <- paste(sep="", testName, "/Results/Summary.data")
   write.table(result, resultsName)
   cat(readLines(pc <- pipe(paste(sep="", "rm -f ", resultsName, ".bz2 ; bzip2 ", resultsName))))
   close(pc)
}


# ====== Variable templates =================================================
perftestPlotVariables <- list(
   # ------ Format example --------------------------------------------------
   # list("Variable",
   #         "Unit[x]{v]"
   #          "100.0 * data1$x / data1$y", <- Manipulator expression:
   #                                           "data" is the data table
   #                                        NA here means: use data1$Variable.
   #          "myColor",
   #          list("InputFile1", "InputFile2", ...))
   #             (measurementDirectory/Results/....data.tar.bz2 is added!)
   # ------------------------------------------------------------------------

   list("ReregistrationRatePerPEandSecond",
           "Reregistration Rate per PE and Second[1/PE*s]",
           "data1$ReregistrationRate / data1$PEs",
           "brown4", list("Summary")),
   list("HandleResolutionRatePerPEandSecond",
           "Handle Resolution Rate per PE and Second[1/PE*s]",
           "data1$HandleResolutionRate / data1$PEs",
           "gold4", list("Summary")),

   list("CPUUtilization",
           "CPU Utilization [%]",
           "100.0 * (data1$SystemTime + data1$UserTime) / data1$Runtime",
           "orange3", list("Summary")),

   list("PEs",
           "Number of Pool Elements{e}[1]",
           NA, "brown4", list("Summary")),
   list("PUs",
           "Number of Pool Users{u}[1]",
           NA, "brown4", list("Summary")),
   list("puToPERatio",
           "PU:PE Ratio{r}[1]",
           "data1$PUs / data1$PEs",
           "brown4", list("Summary")),
   list("ReregInterval",
           "Inter Reregistration Time{r}[s]",
           NA, "brown4", list("Summary")),
   list("InterHResTime",
           "Inter Handle Resolution Time{h}[s]",
           NA, "brown4", list("Summary"))
)
