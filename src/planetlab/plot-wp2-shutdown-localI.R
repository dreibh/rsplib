# ###########################################################################
# Name:        wp2-shutdown-localI
# Description: Cookie-based failover
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "wp2-shutdown-localI"
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
        "Provider's Perspective", NA, NA, list(1,0),
        "CleanShutdownProbability", "calcAppPoolElement-CalcAppPEUtilization",
        "Policy", "PEAllUptime"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA, NA, list(1,0),
        "CleanShutdownProbability", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "PEAllUptime"),

   list(measurementDirectory, paste(sep="", measurementDirectory, "-JobsQueued.pdf"),
        "User's Perspective", NA, NA, list(1,0),
        "CleanShutdownProbability", "calcAppPoolUser-CalcAppPUTotalJobsQueued",
        "Policy", "PEAllUptime"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-JobsStarted.pdf"),
        "User's Perspective", NA, NA, list(1,0),
        "CleanShutdownProbability", "calcAppPoolUser-CalcAppPUTotalJobsStarted",
        "Policy", "PEAllUptime"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-JobsCompleted.pdf"),
        "User's Perspective", NA, NA, list(1,0),
        "CleanShutdownProbability", "calcAppPoolUser-CalcAppPUTotalJobsCompleted",
        "Policy", "PEAllUptime")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
