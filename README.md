
# RgeoProfile
### Workshop branch
[![Build Status](https://travis-ci.org/michael-stevens-27/Rgeoprofile.svg?branch=workshop)](https://travis-ci.org/michael-stevens-27/Rgeoprofile)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/michael-stevens-27/RgeoProfile?branch=workshopr&svg=true)](https://ci.appveyor.com/project/michael-stevens-27/RgeoProfile)

Access documentation [here](https://michael-stevens-27.github.io/Rgeoprofile/) 

## Background
RgeoProfile is an R package for carrying out geographic profiling - a technique derived from criminology that uses the spatial locations of linked crimes to infer the home location (or locations) from which the criminal is operating<sup>1</sup>. More recently, this method has been applied to problems in ecology<sup>2-5</sup> and infectious disease<sup>6,7</sup>, where "crimes" are now equivalent to animal observations or sites of infection, and "home locations" may be roosts or persistent sources of disease. Finding these sources is often a key priority - for example in malaria this may provide a more efficient method of carrying out targeted larval source management<sup>7</sup>.

The RgeoProfile package uses the Dirichlet Process Mixture (DPM) modelling framework of *Verity et al.*<sup>7</sup>, which was designed to place geographic profiling in a Bayesian framework and to deal with the issue of multiple sources. In simple terms, the DPM model assumes a large (strictly infinite) number of *potential* source locations, although some of these sources are assumed to be more "active" than others. The RgeoProfile algorithm then attempts to infer the number of active sources (i.e. the number of sources that gave rise to the observed data), along with the spatial locations of these sources. It does this using Markov chain Monte Carlo (MCMC) - a statistical technique that provides estimates of unknown parameters by iteratively updating our guess.

<p align="center">
<img src="R_ignore/LondonExample_figure1.png" width="700" align="middle">

<p align="center"> Example geoprofile produced by analysing the tutorial data set, included with the package. </p>
</p>

RgeoProfile is written in R, and links to C++ through the Rcpp package. The required data is nothing more than the longitude/latitude of observed "crimes". The user is also required to define a series of parameters, for example defining prior beliefs about the average distance of a crime from a source. After running the main MCMC algorithm, a large number of plotting functions are available for exploring the output and, for evaluating the efficiency of the search strategy.

## References

<sup>1</sup> Rossmo, D.K., 1999. Geographic profiling. CRC press.<br/>
The theory of geographic profiling, including the Criminal Geographic Targeting (CGT) algorithm used in criminology.

<sup>2</sup> Verity R, Stevenson MD, Rossmo DK, Nichols RA, Le Comber SC (2014). Spatial targeting of infectious disease control: Identifying multiple, unknown sources. Methods in Ecology and Evolution vol. 5 (7), 647-655. DOI: 10.1111/2041-210X.12190 <br/>
Introducing the Dirichlet Process Mixture (DPM) model of geographic profiling.

<sup>3</sup> Faulkner SC, Verity R, Roberts D, Roy SS, Robertson PA, Stevenson MD, Le Comber SC (2016). Using geographic profiling to compare the value of sightings vs trap data in a biological invasion. Diversity and Distributions 23 (1), 104-112. DOI: 10.1111/ddi.12498 <br/>
Using citizen science data with geographic profiling, and extending the DPM model to fit sigma, the dispersal parameter.

<sup>4</sup> Faulkner SC, Stevens MCA, Romañach SS, Lindsey PA, Le Comber SC (2018). A spatial approach to combatting wildlife crime. Conservation Biology 32 (3), 685-693. DOI: 10.1111/cobi.13027 <br/>
A study of poaching in Zimbabwe showing how geospatial information can be incorporated within the GP framework.

<sup>5</sup> Struebig MJ, Linkie M, Deere N, Martyr DJ, Millyanawati B, Faulkner SC, Le Comber SC, Mangunjaya FM, Fachruddin M, Leader-Williams N, McKay JE, St John FAV (2018). Addressing human-tiger conflict using socio-ecological information on tolerance and risk. Nature Communications 9 (1), article 3455. DOI: 10.1038/s41467-018-05983-y <br/>
A recent paper using geographic profiling to improve predictions of human-tiger conflict.

<sup>6</sup> Rossmo DK, Lutermann H, Stevenson MD, Le Comber SC (2014). Geographic profiling in Nazi Berlin: fact and fiction. Geospatial Intelligence Review (Fall 2014). <br/>
Applying geographic profiling to a case of German resistance to the Nazis which formed the basis of the classic novel, ‘Alone in Berlin’.
