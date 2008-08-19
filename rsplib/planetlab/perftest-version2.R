# ###########################################################################
# Name:        perftest-version1.R
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")


# ====== Perform handlespace management performance measurement =============
performMeasurement <- function()
{
   prColumn                 <- c()
   peColumn                 <- c()
   puColumn                 <- c()
   primaryPERegistrarColumn <- c()
   primaryPURegistrarColumn <- c()
   policyColumn             <- c()
   reregIntervalColumn      <- c()
   interHResTimeColumn      <- c()
   maxHResItemsColumn       <- c()

   prNumberColumn           <- c()

   registrationRates        <- c()
   reregistrationRates      <- c()
   deregistrationRates      <- c()
   handleResolutionRates    <- c()
   failureReportRates       <- c()
   synchronizationRates     <- c()
   updateRates              <- c()

   runtimeDiffs             <- c()
   userTimeDiffs            <- c()
   systemTimeDiffs          <- c()


   # ------ Create directory and hosts files --------------------------------
   dir.create(testName,showWarnings=FALSE)

   prHostsFile <- paste(sep="", testName, "/PR.hosts")
   fh <- file(prHostsFile, "w")
   for(host in prHosts) {
      cat(sep="", host, "\n", file=fh)
   }
   close(fh)

   peHostsFile <- paste(sep="", testName, "/PE.hosts")
   fh <- file(peHostsFile, "w")
   for(host in peHosts) {
      cat(sep="", host, "\n", file=fh)
   }
   close(fh)

   puHostsFile <- paste(sep="", testName, "/PU.hosts")
   fh <- file(puHostsFile, "w")
   for(host in puHosts) {
      cat(sep="", host, "\n", file=fh)
   }
   close(fh)


   # ------ Create measurement sets -----------------------------------------
   for(PRs in PRsSet) {
   for(PEs in PEsSet) {
   for(puToPERatio in puToPERatioSet) {
   for(PUs in PUsSet) {
   if((length(PUsSet) == 1) && (PUs < 0)) {
      PUs <- PEs * puToPERatio
   }
   for(primaryPERegistrar in primaryPERegistrarSet) {
   for(primaryPURegistrar in primaryPURegistrarSet) {
   for(policy in policySet) {
   for(reregInterval in reregIntervalSet) {
   for(interHResTime in interHResTimeSet) {
   for(maxHResItems in maxHResItemsSet) {

      # ------ Run performance test --------------------------------------------
      duration <- minPreSkip + minPostSkip + (segments + 1) * segmentLength
      runPrefix <- paste(sep="", testName, "-", PRs, "-", PEs, "-", PUs, "-", primaryPERegistrar, "-", primaryPURegistrar, "-", policy, "-", reregInterval, "-", interHResTime, "-", maxHResItems, "-", duration)
      cmdLine <- paste(sep="", "./perftest-version2 ",
                              testName, " ", runPrefix,
                              " ",
                              PRs, " ", PEs, " ", PUs, " ", primaryPERegistrar, " ", primaryPURegistrar, " ", policy, " ",
                              reregInterval, " ", interHResTime, " ", maxHResItems, " ",
                              duration, " ",
                              ">", testName, "/", runPrefix, ".log")
      cat(sep="", "Running ", cmdLine," ...\n")
      cat(readLines(pc <- pipe(cmdLine)))
      close(pc)

      # ------ Collect results -------------------------------------------------
      cat("Collecting results ...\n")
      for(i in seq(1, PRs)) {
         data <- loadResults(paste(sep="", testName, "/", runPrefix, "/TestPR-", i, ".data"))
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
         updateRates  <- append(updateRates,
                              analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                              "Updates", ACRT_Normalized))

         runtimeDiffs <- append(runtimeDiffs,
                              analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                              "Runtime", ACRT_Difference))
         userTimeDiffs <- append(userTimeDiffs,
                              analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                              "UserTime", ACRT_Difference))
         systemTimeDiffs <- append(systemTimeDiffs,
                              analyseCounterResults(data, lowerLimit, upperLimit, segmentLength, segments,
                              "SystemTime", ACRT_Difference))

         prColumn                     <- append(prColumn, rep(PRs, length(registrationResults)))
         peColumn                     <- append(peColumn, rep(PEs, length(registrationResults)))
         puColumn                     <- append(puColumn, rep(PUs, length(registrationResults)))
         primaryPERegistrarColumn     <- append(primaryPERegistrarColumn, rep(primaryPERegistrar, length(registrationResults)))
         primaryPURegistrarColumn     <- append(primaryPURegistrarColumn, rep(primaryPURegistrar, length(registrationResults)))
         policyColumn                 <- append(policyColumn, rep(policy, length(registrationResults)))
         reregIntervalColumn          <- append(reregIntervalColumn, rep(reregInterval, length(registrationResults)))
         interHResTimeColumn          <- append(interHResTimeColumn, rep(interHResTime, length(registrationResults)))
         maxHResItemsColumn           <- append(maxHResItemsColumn,  rep(maxHResItems, length(registrationResults)))
         prNumberColumn               <- append(prNumberColumn, rep(i, length(registrationResults)))
      }
   }}}}}}}}}}


   cat("Writing results ...\n")
   result <- data.frame(prColumn,
                        peColumn,
                        puColumn,
                        reregIntervalColumn,
                        interHResTimeColumn,
                        maxHResItemsColumn,
                        prNumberColumn,
                        primaryPERegistrarColumn,
                        primaryPURegistrarColumn,
                        policyColumn,

                        runtimeDiffs,
                        userTimeDiffs,
                        systemTimeDiffs,

                        registrationRates,
                        reregistrationRates,
                        deregistrationRates,
                        handleResolutionRates,
                        failureReportRates,
                        synchronizationRates,
                        updateRates)
   colnames(result) <- c("PRs", "PEs", "PUs",
                         "ReregInterval", "InterHResTime", "MaxHResItems",
                         "RegistrarNumber", "PrimaryPERegistrar", "PrimaryPURegistrar", "Policy",
                         "Runtime", "UserTime", "SystemTime",
                         "RegistrationRate", "ReregistrationRate", "DeregistrationRate",
                         "HandleResolutionRate", "FailureReportRate",
                         "SynchronizationRate", "UpdateRate")
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
   #          rangeSet: c(minimum, step1, ..., stepN),
   #          rangeColor: c("step1Color", "step2Color", ..., "stepNColor"))
   # ------------------------------------------------------------------------

   list("ReregistrationRatePerPEandSecond",
           "Reregistration Rate per PE and Second[1/PE*s]",
           "data1$ReregistrationRate / data1$PEs",
           "brown4", list("Summary")),
   list("HandleResolutionRatePerPEandSecond",
           "Handle Resolution Rate per PE and Second[1/PE*s]",
           "data1$HandleResolutionRate / data1$PEs",
           "gold4", list("Summary")),
   list("UpdateRatePerPEandSecond",
           "Update Rate per PE and Second[1/PE*s]",
           "data1$UpdateRate / data1$PEs",
           "green4", list("Summary")),
   list("HandleResolutionRatePerPUandSecond",
           "Handle Resolution Rate per PU and Second[1/PU*s]",
           "data1$HandleResolutionRate / data1$PUs",
           "gold2", list("Summary")),
   list("UpdateRatePerPUandSecond",
           "Update Rate per PU and Second[1/PU*s]",
           "data1$UpdateRate / data1$PUs",
           "green2", list("Summary")),

   list("CPUUtilization",
           "CPU Utilization [%]",
           "100.0 * (data1$SystemTime + data1$UserTime) / data1$Runtime",
           "orange3", list("Summary"),
           c(-10, 50, 85, 110), c("#f5fff5", "#fffff0", "#fff0f0")
           ),

   list("PRs",
           "Registrars{G}[1]",
           NA, "brown4", list("Summary")),
   list("PEs",
           "Pool Elements{e}[1]",
           NA, "brown4", list("Summary")),
   list("PUs",
           "Pool Users{u}[1]",
           NA, "brown4", list("Summary")),
   list("RegistrarNumber",
           "Registrar{R}",
           NA, "brown4", list("Summary")),
   list("PrimaryPERegistrar",
           "PEs' Registrar{E}",
           NA, "brown4", list("Summary")),
   list("PrimaryPURegistrar",
           "PUs' Registrar{U}",
           NA, "brown4", list("Summary")),
   list("Policy",
           "Pool Policy{p}",
           NA, "brown4", list("Summary")),
   list("puToPERatio",
           "PU:PE Ratio{r}[1]",
           NA, "brown4", list("Summary")),
   list("ReregInterval",
           "Inter Reregistration Time{g}[s]",
           NA, "brown4", list("Summary")),
   list("MaxHResItems",
           "MaxHResItems{h}[1]",
           NA, "brown4", list("Summary")),
   list("InterHResTime",
           "Inter Handle Resolution Time{a}[s]",
           NA, "brown4", list("Summary"))
)
