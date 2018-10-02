# ###########################################################################
# Name:        plot-paper3
# Description: Paper Plots
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory <- "paper3"
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
plotConfigurations <- list(
   list("tos-pr-failure-localII", "tos-pr-failure-localII-Utilization.pdf",
        "Provider's Perspective", NA, list(seq(0,60,10)), list(1,0),
        "PRAllUptime-Num2to5", "calcAppPoolElement-CalcAppPEUtilization",
        "Policy", "UseTakeoverSuggestion", "",
        "", "", ""),
   list("tos-pr-failure-localII", "tos-pr-failure-localII-HandlingSpeed.pdf",
        "User's Perspective", NA, list(seq(20,90,10)), list(0.5,1),
        "PRAllUptime-Num2to5", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "UseTakeoverSuggestion", "",
        "", "", ""),

   list("tos-pr-failure-localII", "tos-pr-failure-localII-RegistrarAverageNumberOfPoolElements.pdf",
        "Providers's Perspective", NA, list(seq(0,24,2)), list(1,0),
        "PRAllUptime-Num2to5", "registrar-RegistrarAverageNumberOfPoolElements",
        "registrar", "UseTakeoverSuggestion", "",
        "", "", "",
        "(data1$registrar == 1)|(data1$registrar == 5)"),
   list("tos-pr-failure-localII", "tos-pr-failure-localII-RegistrarAverageNumberOfOwnedPoolElements.pdf",
        "Providers's Perspective", NA, list(seq(0,24,2)), list(1,0.75),
        "PRAllUptime-Num2to5", "registrar-RegistrarAverageNumberOfOwnedPoolElements",
        "registrar", "UseTakeoverSuggestion", "",
        "", "", "",
        "(data1$registrar == 1)|(data1$registrar == 5)"),

   list("tos-pr-failure-localII", "tos-pr-failure-localII-RegistrarTotalEndpointKeepAlivesSent.pdf",
        "Providers's Perspective", NA, NA, list(1,0.5),
        "PRAllUptime-Num2to5", "registrar-RegistrarTotalEndpointKeepAlives",
        "registrar", "UseTakeoverSuggestion", "",
        "", "", "",
        "(data1$registrar == 1)|(data1$registrar == 5)"),
   list("tos-pr-failure-localII", "tos-pr-failure-localII-RegistrarTotalHandleUpdates.pdf",
        "Providers's Perspective", NA, NA, list(1,0.5),
        "PRAllUptime-Num2to5", "registrar-RegistrarTotalHandleUpdates",
        "registrar", "UseTakeoverSuggestion", "Policy",
        "", "", "",
        "(data1$Policy==\"LeastUsed\")&((data1$registrar == 1)|(data1$registrar == 5))"),
   list("tos-pr-failure-localII", "tos-pr-failure-localII-RegistrarTotalRegistrations.pdf",
        "Providers's Perspective", NA, NA, list(1,0.7),
        "PRAllUptime-Num2to5", "registrar-RegistrarTotalRegistrations",
        "registrar", "UseTakeoverSuggestion", "Policy",
        "", "", "",
        "(data1$Policy==\"LeastUsed\")&((data1$registrar == 1)|(data1$registrar == 5))")
)

# ------ Variable templates -------------------------------------------------
plotVariables <- append(list(
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

   list("PRAllUptime-Num2to5",
          "Registrar MTBF for PR #2 to #5 {M}[s]",
          "data1$PRAllUptime", "brown4")

), plotVariables)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
