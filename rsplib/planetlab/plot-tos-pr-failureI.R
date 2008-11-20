# ###########################################################################
# Name:        tos-pr-failureI
# Description:
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory  <- "tos-pr-failureI"
plotColorMode         <- cmColor
plotHideLegend        <- FALSE
plotLegendSizeFactor  <- 0.8
plotOwnOutput         <- FALSE
plotFontFamily        <- "Helvetica"
plotFontPointsize     <- 22
plotWidth             <- 20
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

   list(measurementDirectory, paste(sep="", measurementDirectory, "-RegistrarAverageNumberOfPoolElements.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PRAllUptime", "registrar-RegistrarAverageNumberOfPoolElements",
        "Policy", "registrar", "", "UseTakeoverSuggestion"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-RegistrarAverageNumberOfOwnedPoolElements.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PRAllUptime", "registrar-RegistrarAverageNumberOfOwnedPoolElements",
        "Policy", "registrar", "", "UseTakeoverSuggestion"),

   list(measurementDirectory, paste(sep="", measurementDirectory, "-RegistrarTotalEndpointKeepAlives.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PRAllUptime", "registrar-RegistrarTotalRegistrations",
        "Policy", "registrar", "", "UseTakeoverSuggestion"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-RegistrarTotalEndpointKeepAlives.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PRAllUptime", "registrar-RegistrarTotalReregistrations",
        "Policy", "registrar", "", "UseTakeoverSuggestion"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-RegistrarTotalEndpointKeepAlives.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PRAllUptime", "registrar-RegistrarTotalEndpointKeepAlives",
        "Policy", "registrar", "", "UseTakeoverSuggestion"),
   list(measurementDirectory, paste(sep="", measurementDirectory, "-RegistrarTotalEndpointKeepAlives.pdf"),
        "Provider's Perspective", NA, NA, list(1,1),
        "PRAllUptime", "registrar-RegistrarTotalHandleResolutions",
        "Policy", "registrar", "", "UseTakeoverSuggestion")

#    list(measurementDirectory, paste(sep="", measurementDirectory, "-Utilization.pdf"),
#         "Provider's Perspective", NA, list(seq(0,60,10)), list(1,0),
#         "AttackInterval", "calcAppPoolElement-CalcAppPEUtilization",
#         "Policy", ""),
#    list(measurementDirectory, paste(sep="", measurementDirectory, "-HandlingSpeed.pdf"),
#         "User's Perspective", NA, list(seq(0,80,10)), list(1,0),
#         "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
#         "Policy", "")

#    list(measurementDirectory, paste(sep="", measurementDirectory, "-CalcAppPUTotalJobsQueued.pdf"),
#         "User's Perspective", NA, NA, list(0,1),
#         "AttackInterval", "calcAppPoolUser-CalcAppPUTotalJobsQueued",
#         "Policy", ""),
#    list(measurementDirectory, paste(sep="", measurementDirectory, "-CalcAppPUTotalJobsStarted.pdf"),
#         "User's Perspective", NA, NA, list(0,1),
#         "AttackInterval", "calcAppPoolUser-CalcAppPUTotalJobsStarted",
#         "Policy", ""),
#    list(measurementDirectory, paste(sep="", measurementDirectory, "-CalcAppPUTotalJobsCompleted.pdf"),
#         "User's Perspective", NA, NA, list(0,1),
#         "AttackInterval", "calcAppPoolUser-CalcAppPUTotalJobsCompleted",
#         "Policy", "")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
