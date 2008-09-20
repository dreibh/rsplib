# ###########################################################################
# Name:        attack-plottertemplates.R
# Description: Templates for plotting attack scenario results
# Revision:    $Id: plot-attack-countermeasure1.R 1923 2008-09-20 04:29:19Z dreibh $
# ###########################################################################

source("plotter.R")


# ------ Variable templates -------------------------------------------------
plotVariables <- list(
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

   list("calcAppPoolElement-CalcAppPEUtilization",
           "Average Request Handling Speed [%]",
           "100.0 * data1$calcAppPoolElement.CalcAppPEUtilization",
           "blue4",
           list("calcAppPoolElement-CalcAppPEUtilization")),
   list("calcAppPoolUser-CalcAppPUAverageHandlingSpeed",
           "Average Request Handling Speed [Calculations/s]",
           "data1$calcAppPoolUser.CalcAppPUAverageHandlingSpeed",
           "brown4",
           list("calcAppPoolUser-CalcAppPUAverageHandlingSpeed")),
   list("calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
           "Average Request Handling Speed [%]",
           "100.0 * data1$calcAppPoolUser.CalcAppPUAverageHandlingSpeed / data1$Capacity",
           "brown4",
           list("calcAppPoolUser-CalcAppPUAverageHandlingSpeed")),

   list("calcAppPoolUser-CalcAppPUTotalJobsQueued",
           "Total Requests Queued [Requests/PU]",
           "data1$calcAppPoolUser.CalcAppPUTotalJobsQueued",
           "blue4",
           list("calcAppPoolUser-CalcAppPUTotalJobsQueued")),
   list("calcAppPoolUser-CalcAppPUTotalJobsStarted",
           "Total Requests Started [Requests/PU]",
           "data1$calcAppPoolUser.CalcAppPUTotalJobsStarted",
           "yellow4",
           list("calcAppPoolUser-CalcAppPUTotalJobsStarted")),
   list("calcAppPoolUser-CalcAppPUTotalJobsCompleted",
           "Total Requests Completed [Requests/PU]",
           "data1$calcAppPoolUser.CalcAppPUTotalJobsCompleted",
           "green4",
           list("calcAppPoolUser-CalcAppPUTotalJobsCompleted")),

   list("Policy",
           "Pool Policy{P}",
           NA, "black"),
   list("AttackInterval",
           "Attack Interval{A}[s]",
           NA, "black"),
   list("AttackReportUnreachableProbability",
           "AttackReportUnreachableProbability{u}[%]",
           "100.0 * data1$AttackReportUnreachableProbability", "black")
)
