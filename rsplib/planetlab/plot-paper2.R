# ###########################################################################
# Name:        plot-paper2
# Description: Paper Plots
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
measurementDirectory <- "paper2"
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
   list("attack-registration-planetlabII", "attack-registration-planetlabII-HandlingSpeed.pdf",
        "Pool Element Attack (PlanetLab)", list(seq(0,1,0.1)), list(seq(0,85,10)), list(1,0),
        "AttackFrequency", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", ""),
   list("attack-registration-countermeasure-planetlabIII", "attack-registration-countermeasure-planetlabIII-HandlingSpeed.pdf",
        "PlanetLab", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackFrequency"),

   list("attack-handleResolution-planetlabII", "attack-handleResolution-planetlabII-HandlingSpeed.pdf",
        "Pool User Attack (PlanetLab)", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "AttackFrequency", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability"),
   list("attack-handleResolution-countermeasure-planetlabII", "attack-handleResolution-countermeasure-planetlabII-HandlingSpeed.pdf",
        "PlanetLab", list(seq(0,2,0.1)),  list(seq(0,85,10)), list(1,0),
        "AttackFrequency", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability"),
   list("attack-handleResolution-countermeasure-planetlabIII", "attack-handleResolution-countermeasure-planetlabIII-HandlingSpeed.pdf",
        "PlanetLab", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability", "AttackFrequency")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
