# ###########################################################################
# Name:        attack-plottertemplates.R
# Description: Templates for plotting attack scenario results
# Revision:    $Id$
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

   list("registrar-RegistrarAverageNumberOfPoolElements",
           "Average Number of PEs [1]",
           "data1$registrar.RegistrarAverageNumberOfPoolElements",
           "yellow4",
           list("registrar-RegistrarAverageNumberOfPoolElements")),
   list("registrar-RegistrarAverageNumberOfOwnedPoolElements",
           "Average Number of Owned PEs [1]",
           "data1$registrar.RegistrarAverageNumberOfOwnedPoolElements",
           "green4",
           list("registrar-RegistrarAverageNumberOfOwnedPoolElements")),

   list("registrar-RegistrarTotalEndpointKeepAlives",
           "Number of Endpoint Keep-Alives [1]",
           "data1$registrar.RegistrarTotalEndpointKeepAlives",
           "yellow4",
           list("registrar-RegistrarTotalEndpointKeepAlives")),
   list("registrar-RegistrarTotalRegistrations",
           "Number of Registrations [1]",
           "data1$registrar.RegistrarTotalRegistrations",
           "yellow4",
           list("registrar-RegistrarTotalRegistrations")),
   list("registrar-RegistrarTotalReregistrations",
           "Number of Reregistrations [1]",
           "data1$registrar.RegistrarTotalReregistrations",
           "yellow4",
           list("registrar-RegistrarTotalReregistrations")),
   list("registrar-RegistrarTotalHandleResolutions",
           "Number of Handle Resolutions [1]",
           "data1$registrar.RegistrarTotalHandleResolutions",
           "yellow4",
           list("registrar-RegistrarTotalHandleResolutions")),
   list("registrar-RegistrarTotalHandleUpdates",
           "Number of Handle Updatess [1]",
           "data1$registrar.RegistrarTotalHandleUpdates",
           "yellow4",
           list("registrar-RegistrarTotalHandleUpdates")),

   list("registrar-X",
           "Average Number of PEs [1]",
           "data1$registrar.X",
           "yellow4",
           list("registrar-X")),


   list("calcAppPoolElement-CalcAppPEUtilization",
           "Average System Utilization [%]",
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

   list("PRAllUptime",
           "PR MTBF{M}",
           NA, "black"),

   list("Policy",
           "Pool Policy{p}",
           NA, "black"),
   list("Attackers",
           "Number of Attackers{:alpha:}[1]",
           NA, "black"),
   list("AttackInterval",
           "Attack Interval{A}[s]",
           NA, "black"),
   list("AttackFrequency",
           "Attack Frequency{F}[1/s]",
           "1.0 / data1$AttackInterval", "black"),
   list("AttackReportUnreachableProbability",
           "Unreachable Probability{u}[%]",
           "100.0 * data1$AttackReportUnreachableProbability", "black"),
   list("UseTakeoverSuggestion",
           "Use Takeover Suggestion {:tau:}",
           "data1$UseTakeoverSuggestion", "black"),

   list("registrar",
           "Registrar Number {:rho:}[#]",
           "data1$registrar", "black")
)
