# ###########################################################################
# Name:        plot-test33
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")

# ------ Plotter Settings ---------------------------------------------------
simulationDirectory  <- "P01"
plotColorMode        <- cmColor
plotHideLegend       <- FALSE
plotLegendSizeFactor <- 0.8
plotOwnOutput        <- FALSE
plotFontFamily       <- "Helvetica"
plotFontPointsize    <- 22
plotWidth            <- 10
plotHeight           <- 10
plotConfidence       <- 0.95

# ###########################################################################

# ------ Plots --------------------------------------------------------------
plotConfigurations <- list(
   # ------ Format example --------------------------------------------------
   # list(simulationDirectory, "output.pdf",
   #      "Plot Title",
   #      list(xAxisTicks) or NA, list(yAxisTicks) or NA, list(legendPos) or NA,
   #      "x-Axis Variable", "y-Axis Variable",
   #      "z-Axis Variable", "v-Axis Variable", "w-Axis Variable",
   #      "a-Axis Variable", "b-Axis Variable", "p-Axis Variable")
   # ------------------------------------------------------------------------

   # ------ Request Interval/PU:PE Ratio -> Request Size --------------------
   list(simulationDirectory, paste(sep="", simulationDirectory, "-Utilization.pdf"),
        "Provider's Perspective", NA, NA, NA,
        "ReregInterval", "ReregistrationRate",
        "PEs")
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
   #             (simulationDirectory/Results/....data.tar.bz2 is added!)
   # ------------------------------------------------------------------------

   list("ReregistrationRate",
           "Reregistration Rate[1/s]",
           NA, "brown4", list("Summary")),

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

createPlots(simulationDirectory, plotConfigurations)
