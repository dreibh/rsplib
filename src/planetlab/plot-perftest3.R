# ###########################################################################
# Name:        plot-perftest3.R
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version2.R")

# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "P03"
plotColorMode         <- cmColor
plotHideLegend        <- FALSE
plotLegendSizeFactor  <- 0.8
plotOwnOutput         <- TRUE
plotFontFamily        <- "Helvetica"
plotFontPointsize     <- 22
plotWidth             <- 10
plotHeight            <- 10
plotConfidence        <- 0.95

# ###########################################################################

filter <- "((data1$RegistrarNumber == 1) | (data1$RegistrarNumber == 2)) &
           (!( (data1$PRs == 7) & (data1$ReregInterval == 250) & (data1$RegistrarNumber == 1) )) &
           (!( (data1$PRs == 8) & (data1$ReregInterval == 250) & (data1$RegistrarNumber == 2) ))"

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
        "PRs", "ReregistrationRatePerPEandSecond", "RegistrarNumber", "ReregInterval", "",
        "", "", "", filter),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-CPUUtilization.pdf"),
        "Provider's Perspective", NA, list(seq(0, 100, 20)), list(0,1),
        "PRs", "CPUUtilization", "RegistrarNumber", "ReregInterval", "",
        "", "", "", filter)
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
