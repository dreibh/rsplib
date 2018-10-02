# ###########################################################################
# Name:        attack-handleResolution-countermeasure-localII
# Description: PU-based attack countermeasure (localhost measurements)
#              Varying the number of attackers
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "attack-handleResolution-countermeasure-localII"
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

   list(measurementDirectory, paste(sep="", measurementDirectory, "-Utilization.pdf"),
        "Provider's Perspective", NA, list(seq(0,60,10)), list(1,0),
        "Attackers", "calcAppPoolElement-CalcAppPEUtilization",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA,  list(seq(0,80,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval"),

   list(measurementDirectory, paste(sep="", measurementDirectory, "-CalcAppPUTotalJobsQueued.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "Attackers", "calcAppPoolUser-CalcAppPUTotalJobsQueued",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-CalcAppPUTotalJobsStarted.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "Attackers", "calcAppPoolUser-CalcAppPUTotalJobsStarted",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-CalcAppPUTotalJobsCompleted.pdf"),
        "User's Perspective", NA, NA, list(0,1),
        "Attackers", "calcAppPoolUser-CalcAppPUTotalJobsCompleted",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
