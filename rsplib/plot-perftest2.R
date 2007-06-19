# ###########################################################################
# Name:        plot-perftest1.R
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version2.R")

# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "P02"
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
filterRulePR1 <- "(data1$PrimaryPURegistrar == 1)"
filterRulePR2 <- "(data1$PrimaryPURegistrar == 2)"

plotConfigurations <- list(
   # ------ Format example --------------------------------------------------
   # list(measurementDirectory, "output.pdf",
   #      "Plot Title",
   #      list(xAxisTicks) or NA, list(yAxisTicks) or NA, list(legendPos) or NA,
   #      "x-Axis Variable", "y-Axis Variable",
   #      "z-Axis Variable", "v-Axis Variable", "w-Axis Variable",
   #      "a-Axis Variable", "b-Axis Variable", "p-Axis Variable",
   #      "(data1$XY == xy) && (data1$YZ = yz)")
   # ------------------------------------------------------------------------

   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandleResolutionRate.pdf"),
        "Pool User's Perspective using PR #1", NA, NA, list(0.2,0.0),
        "PUs", "HandleResolutionRatePerPUandSecond",
        "RegistrarNumber", "InterHResTime", "PEs",
        "", "", "", filterRulePR1),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandleResolutionRate.pdf"),
        "Pool User's Perspective using PR #2", NA, NA, list(0.2,0.0),
        "PUs", "HandleResolutionRatePerPUandSecond",
        "RegistrarNumber", "InterHResTime", "PEs",
        "", "", "", filterRulePR2),

   list(measurementDirectory, paste(sep="", measurementDirectory, "-CPUUtilization.pdf"),
        "Registrar's Perspective using PR #1", NA, NA, list(0,1),
        "PUs", "CPUUtilization",
        "RegistrarNumber", "InterHResTime", "PEs",
        "", "", "", filterRulePR1),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-CPUUtilization.pdf"),
        "Registrar's Perspective using PR #2", NA, NA, list(0,1),
        "PUs", "CPUUtilization",
        "RegistrarNumber", "InterHResTime", "PEs",
        "", "", "", filterRulePR2)
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
