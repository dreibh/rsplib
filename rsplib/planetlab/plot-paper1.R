# ###########################################################################
# Name:        plot-paper1
# Description: Paper Plots
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
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
   list("attack-registration-localI", "attack-registration-localI-HandlingSpeed.pdf",
        "Pool Element Attack (Lab Measurement)", NA, list(seq(0,85,10)), list(1,0),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", ""),
   list("attack-registration-countermeasure-localII", "attack-registration-countermeasure-localII-HandlingSpeed.pdf",
        "Lab Measurement", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackInterval"),

   list("attack-handleResolution-localI", "attack-handleResolution-localI-HandlingSpeed.pdf",
        "Pool User Attack (Lab Measurement)", NA,  list(seq(0,85,10)), list(1,0),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability"),
   list("attack-handleResolution-countermeasure-localI", "attack-handleResolution-countermeasure-localI-HandlingSpeed.pdf",
        "Lab Measurement", NA,  list(seq(0,85,10)), list(1,0),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability"),
   list("attack-handleResolution-countermeasure-localII", "attack-handleResolution-countermeasure-localII-HandlingSpeed.pdf",
        "Lab Measurement", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
