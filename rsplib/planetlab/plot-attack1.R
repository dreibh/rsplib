# ###########################################################################
# Name:        attack1
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "attack1"
plotColorMode         <- cmColor
plotHideLegend        <- FALSE
plotLegendSizeFactor  <- 0.8
plotOwnOutput         <- FALSE
plotFontFamily        <- "Helvetica"
plotFontPointsize     <- 22
plotWidth             <- 10
plotHeight            <- 10
plotConfidence        <- 0.95

# ###########################################################################

# ------ Plots --------------------------------------------------------------
plotConfigurations <- list(
   # ------ Format example --------------------------------------------------
   # list(measurementDirectory, "output.pdf",
   #      "Plot Title",
   #      list(xAxisTicks) or NA, list(yAxisTicks) or NA, list(legendPos) or NA,
   #      "x-Axis Variable", "y-Axis Variable",
   #      "z-Axis Variable", "v-Axis Variable", "w-Axis Variable",
   #      "a-Axis Variable", "b-Axis Variable", "p-Axis Variable")
   # ------------------------------------------------------------------------

   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", ""),

   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "AttackInterval", "calcAppPoolUser-CalcAppPUTotalJobsQueued",
        "Policy", ""),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "AttackInterval", "calcAppPoolUser-CalcAppPUTotalJobsStarted",
        "Policy", ""),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "AttackInterval", "calcAppPoolUser-CalcAppPUTotalJobsCompleted",
        "Policy", "")
)


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

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
