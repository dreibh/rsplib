# ###########################################################################
# Name:        plot-perftest1.R
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version1.R")

# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "P01"
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

   list(measurementDirectory, paste(sep="", measurementDirectory, "-ReregistrationRate.pdf"),
        "Provider's Perspective", NA, NA, list(0,0),
        "PEs", "ReregistrationRatePerPEandSecond", "ReregInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandleResolutionRate.pdf"),
        "Provider's Perspective", NA, NA, list(0,1),
        "PEs", "HandleResolutionRatePerPEandSecond", "ReregInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-CPUUtilization.pdf"),
        "Provider's Perspective", NA, list(seq(0, 100, 20)), list(0,1),
        "PEs", "CPUUtilization", "ReregInterval")
)


# ------ Variable templates -------------------------------------------------
plotVariables <- append(perftestPlotVariables, list(
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

))

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
