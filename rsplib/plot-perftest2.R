# ###########################################################################
# Name:        plot-perftest1.R
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")

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
        "Provider's Perspective", NA, NA, list(1,1),
        "PEs", "ReregistrationRatePerPEandSecond", "ReregInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-HandleResolutionRate.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PEs", "HandleResolutionRatePerPEandSecond", "ReregInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-CPUUtilization.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PEs", "CPUUtilization", "ReregInterval")
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

   list("ReregistrationRatePerPEandSecond",
           "Reregistration Rate per PE and Second[1/PE*s]",
           "data1$ReregistrationRate / data1$PEs",
           "brown4", list("Summary")),
   list("HandleResolutionRatePerPEandSecond",
           "Handle Resolution Rate per PE and Second[1/PE*s]",
           "data1$HandleResolutionRate / data1$PEs",
           "gold4", list("Summary")),

   list("CPUUtilization",
           "CPU Utilization [%]",
           "100.0 * (data1$SystemTime + data1$UserTime) / data1$Runtime",
           "orange3", list("Summary")),

   list("PEs",
           "Number of Pool Elements{e}[1]",
           NA, "brown4", list("Summary")),
   list("PUs",
           "Number of Pool Users{u}[1]",
           NA, "brown4", list("Summary")),
   list("ReregInterval",
           "Inter Reregistration Time{r}[s]",
           NA, "brown4", list("Summary")),
   list("InterHResTime",
           "Inter Handle Resolution Time{h}[s]",
           NA, "brown4", list("Summary"))
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
