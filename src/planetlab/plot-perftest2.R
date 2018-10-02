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
plotOwnOutput         <- TRUE
plotFontFamily        <- "Helvetica"
plotFontPointsize     <- 22
plotWidth             <- 10
plotHeight            <- 10
plotConfidence        <- 0.95

# ###########################################################################

# ------ Plots --------------------------------------------------------------
sed <- "sed -e \"s/\\\"RoundRobin\\\"/\\\"deterministic\\\"/g\" -e \"s/\\\"Random\\\"/\\\"randomized\\\"/g\""

filterRulePR1 <- "( (data1$PrimaryPURegistrar == 1) & ((data1$RegistrarNumber == 1) | ((data1$Policy == \"deterministic\") & (data1$InterHResTime == 100)) ) )"
filterRulePR2 <- "( (data1$PrimaryPURegistrar == 2) & ((data1$RegistrarNumber == 2) | ((data1$Policy == \"deterministic\") & (data1$InterHResTime == 100)) ) )"

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

   list(measurementDirectory, paste(sep="", measurementDirectory, "-PR1HandleResolutionRate.pdf"),
        "Pool User's Perspective using PR #1", NA, NA, list(0.2,0.0),
        "PUs", "HandleResolutionRatePerPUandSecond",
        "RegistrarNumber", "Policy", "InterHResTime",
        "", "", "", filterRulePR1),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-PR2HandleResolutionRate.pdf"),
        "Pool User's Perspective using PR #2", NA, NA, list(0.2,0.0),
        "PUs", "HandleResolutionRatePerPUandSecond",
        "RegistrarNumber", "Policy", "InterHResTime",
        "", "", "", filterRulePR2),

   list(measurementDirectory, paste(sep="", measurementDirectory, "-PR1CPUUtilization.pdf"),
        "Registrar's Perspective using PR #1", NA, list(seq(0, 100, 20)), list(0,1),
        "PUs", "CPUUtilization",
        "RegistrarNumber", "Policy", "InterHResTime",
        "", "", "", filterRulePR1),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-PR2CPUUtilization.pdf"),
        "Registrar's Perspective using PR #2", NA, list(seq(0, 100, 20)), list(0,1),
        "PUs", "CPUUtilization",
        "RegistrarNumber", "Policy", "InterHResTime",
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

createPlots(measurementDirectory, plotConfigurations, sed)
