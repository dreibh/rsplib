# ###########################################################################
# Name:        wp2-dynpool-mtbf-localI
# Description: PE MTBF Variation
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "wp2-dynpool-mtbf-localI"
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
        "PEAllUptime", "calcAppPoolElement-CalcAppPEUtilization",
        "Policy", "CleanShutdownProbability"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
        "User's Perspective", NA, NA, list(1,0),
        "PEAllUptime", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "CleanShutdownProbability")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
