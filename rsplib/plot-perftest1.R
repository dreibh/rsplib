# ###########################################################################
# Name:        plot-test33
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")

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
        "Provider's Perspective", NA, NA, list(0,1),
        "PEs", "ReregistrationRate", "ReregInterval"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-ReregistrationRate.pdf"),
        "Provider's Perspective", NA, NA, list(0,0),
        "PEs", "HandleResolutionRate", "ReregInterval")
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

   list("ReregistrationRate",
           "Reregistration Rate[1/s]",
           NA, "brown4", list("Summary")),
   list("HandleResolutionRate",
           "Handle Resolution Rate[1/s]",
           NA, "gold4", list("Summary")),

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
