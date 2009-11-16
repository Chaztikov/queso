/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------
 *
 * Copyright (C) 2008 The PECOS Development Team
 *
 * Please see http://pecos.ices.utexas.edu for more information.
 *
 * This file is part of the QUESO Library (Quantification of Uncertainty
 * for Estimation, Simulation and Optimization).
 *
 * QUESO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUESO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUESO. If not, see <http://www.gnu.org/licenses/>.
 *
 *--------------------------------------------------------------------------
 *
 * $Id$
 *
 * Brief description of this file: 
 * 
 *--------------------------------------------------------------------------
 *-------------------------------------------------------------------------- */

#ifndef __UQ_MH_SG2_H__
#define __UQ_MH_SG2_H__

/*! Operation to generate the chain */
/*! Requirements:
<list type=number>
<item> the vector space 'm_vectorSpace' should have dimension equal to the size of a vector in 'workingChain'
</list>
*/
/*! If the requirements are satisfied, this operation sets the size and the contents of 'workingChain' using the algorithm options set in the constructor.
    If not NULL, 'workingLogLikelihoodValues' and 'workingLogTargetValues' are set accordingly.
*/
/*! If options request data to be written in the output file (MATLAB .m format only, for now), the user can check which MATLAB variables are defined and set by
    running 'grep zeros <OUTPUT FILE NAME>' after the solution procedure ends. THe names of the varibles are self explanatory.
*/
/*  This operation currently implements the DRAM algorithm (Heikki Haario, Marko Laine, Antonietta Mira and
    Eero Saksman, "DRAM: Efficient Adaptive MCMC", Statistics and Computing (2006), 16:339-354).
    It also provides support for Stochastic Newton algorithm through the TK (transition kernel) class.
    Stochastic Newton is not totally implemented yet though, since it is being researched by James Martin and Omar Ghattas at ICES.
*/
/*! -------------------------------------------------------------
*/
/*! Acknowledgments: this operation 'generateSequence()' begun on July of 2008 as a translation
    of the core routine at the MCMC toolbox for MATLAB, available at www.helsinki.fi/~mjlaine/mcmc/.
    Indeed, the example available in examples/statisticalInverseProblem1/tests/test_2009_02_03/ is
    related to the 'normal example' in the toolbox.
    Over time, though:
<list type=number>
<item> the whole set of QUESO classes took shape, focusing not only on Markov Chains, but on statistical forward problems and model validation as well;
<item> the interfaces to this Metropolis-Hastings class changed;
<item> QUESO had parallel capabilities;
<item> we added the TK class, as mentioned above, in order to have both DRAM with Stochastic Newton capabilities.
</list>
*/
template <class P_V,class P_M>
void
uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence(
  uqBaseVectorSequenceClass<P_V,P_M>& workingChain,
  uqScalarSequenceClass<double>*      workingLogLikelihoodValues,
  uqScalarSequenceClass<double>*      workingLogTargetValues)
{
  if ((m_env.subDisplayFile()          ) &&
      (m_env.displayVerbosity() >= 5   ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "Entering uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()..."
                            << std::endl;
  }

  UQ_FATAL_TEST_MACRO(m_vectorSpace.dimLocal() != workingChain.vectorSizeLocal(),
                      m_env.fullRank(),
                      "uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()",
                      "'m_vectorSpace' and 'workingChain' are related to vector spaces of different dimensions");

  //m_env.syncPrintDebugMsg("Entering uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()",2,3000000,m_env.fullComm());  // Dangerous to barrier on fullComm ... // KAUST
  uqMiscCheckTheParallelEnvironment<P_V,P_V>(m_initialPosition,
                                             m_initialPosition);

  P_V valuesOf1stPosition(m_initialPosition);
  int iRC = UQ_OK_RC;

  workingChain.setName(m_options.m_prefix + "rawChain");

  //****************************************************
  // Generate chain
  //****************************************************
  if (m_options.m_rawChainDataInputFileName == UQ_MH_SG_FILENAME_FOR_NO_FILE) {
    generateFullChain(valuesOf1stPosition,
                      m_options.m_rawChainSize,
                      workingChain,
                      workingLogLikelihoodValues,
                      workingLogTargetValues);
  }
  else {
    readFullChain(m_options.m_rawChainDataInputFileName,
                  m_options.m_rawChainSize,
                  workingChain);
  }

  //****************************************************
  // Open generic output file
  //****************************************************
  if ((m_env.subDisplayFile()          ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                            << ", prefix = "                                                         << m_options.m_prefix
                            << ": checking necessity of opening generic output file (chain name is " << workingChain.name()
                            << ") ..."
                            << std::endl;
  }
  std::ofstream* genericOfsVar = NULL;
  m_env.openOutputFile(m_options.m_dataOutputFileName,
                       UQ_FILE_EXTENSION_FOR_MATLAB_FORMAT,
                       m_options.m_dataOutputAllowedSet,
                       false,
                       genericOfsVar);

  //****************************************************
  // Eventually:
  // --> write raw chain
  // --> compute statistics on it
  //****************************************************
  if ((m_env.subDisplayFile()          ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                            << ", prefix = "                                                 << m_options.m_prefix
                            << ": checking necessity of opening output files for raw chain " << workingChain.name()
                            << "..."
                            << std::endl;
  }

  // Take "sub" care of raw chain
  if ((m_options.m_rawChainDataOutputFileName != UQ_MH_SG_FILENAME_FOR_NO_FILE) &&
      (m_options.m_totallyMute == false                                       )) {
    workingChain.subWriteContents(m_options.m_rawChainDataOutputFileName,
                                  m_options.m_rawChainDataOutputAllowedSet);
    if ((m_env.subDisplayFile()          ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                              << ", prefix = "                << m_options.m_prefix
                              << ": closed sub output file '" << m_options.m_rawChainDataOutputFileName
                              << "' for raw chain "           << workingChain.name()
                              << std::endl;
    }
  }

  // Take "unified" care of raw chain
  if ((m_options.m_rawChainDataOutputFileName != UQ_MH_SG_FILENAME_FOR_NO_FILE) &&
      (m_options.m_totallyMute == false                                       )) {
    workingChain.unifiedWriteContents(m_options.m_rawChainDataOutputFileName);
    if ((m_env.subDisplayFile()          ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                              << ", prefix = "                    << m_options.m_prefix
                              << ": closed unified output file '" << m_options.m_rawChainDataOutputFileName
                              << "' for raw chain "               << workingChain.name()
                              << std::endl;
    }
  }

  // Take care of other aspects of raw chain
  if ((genericOfsVar                   ) &&
      (m_options.m_totallyMute == false)) {
    // Write likelihoodValues and alphaValues, if they were requested by user
    iRC = writeInfo(workingChain,
                    *genericOfsVar);
    UQ_FATAL_RC_MACRO(iRC,
                      m_env.fullRank(),
                      "uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()",
                      "improper writeInfo() return");
  }

  if (m_options.m_rawChainComputeStats) {
    workingChain.computeStatistics(*m_options.m_rawChainStatisticalOptions,
                                   genericOfsVar);
  }

#if 0 // 2009 03 29, prudenci: do not throw code away yet; just comment code for now
  //****************************************************
  // Eventually:
  // --> generate unique chain
  // --> write it
  // --> compute statistics on it
  //****************************************************
  if (m_uniqueChainGenerate) {
    std::ofstream* uniqueChainOfsVar = NULL;

    // Select only the unique positions
    workingChain.select(m_idsOfUniquePositions);
    //chainVectorPositionIteratorTypedef positionIterator = m_uniqueChain1.begin();
    //std::advance(positionIterator,uniquePos);
    //m_uniqueChain1.erase(positionIterator,m_uniqueChain1.end());
    //UQ_FATAL_TEST_MACRO((uniquePos != m_uniqueChain1.size()),
    //                    m_env.fullRank(),
    //                    "uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()",
    //                    "uniquePos != m_uniqueChain1.size()");

    // Write unique chain
    workingChain.setName(m_options.m_prefix + "uniqueChain");
    if (uniqueChainOfsVar) {
      workingChain.subWriteContents(*uniqueChainOfsVar);
    }

    // Compute statistics
    if (m_uniqueChainComputeStats) {
      workingChain.computeStatistics(*m_uniqueChainStatisticalOptions,
                                     genericOfsVar);
    }
  }
#endif

  //****************************************************
  // Eventually:
  // --> filter the raw chain
  // --> write it
  // --> compute statistics on it
  //****************************************************
  if (m_options.m_filteredChainGenerate) {
    // Compute filter parameters
    unsigned int filterInitialPos = (unsigned int) (m_options.m_filteredChainDiscardedPortion * (double) workingChain.subSequenceSize());
    unsigned int filterSpacing    = m_options.m_filteredChainLag;
    if (filterSpacing == 0) {
      workingChain.computeFilterParams(*m_options.m_filteredChainStatisticalOptions,
                                       genericOfsVar,
                                       filterInitialPos,
                                       filterSpacing);
    }

    // Filter positions from the converged portion of the chain
    workingChain.filter(filterInitialPos,
                        filterSpacing);
    workingChain.setName(m_options.m_prefix + "filtChain");

    if (workingLogLikelihoodValues) workingLogLikelihoodValues->filter(filterInitialPos,
                                                                       filterSpacing);

    if (workingLogTargetValues) workingLogTargetValues->filter(filterInitialPos,
                                                               filterSpacing);

    // Write filtered chain
    if ((m_env.subDisplayFile()          ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                              << ", prefix = "                                                      << m_options.m_prefix
                              << ": checking necessity of opening output files for filtered chain " << workingChain.name()
                              << "..."
                              << std::endl;
    }

    // Take "sub" care of filtered chain
    if ((m_options.m_filteredChainDataOutputFileName != UQ_MH_SG_FILENAME_FOR_NO_FILE) &&
        (m_options.m_totallyMute == false                                            )) {
      workingChain.subWriteContents(m_options.m_filteredChainDataOutputFileName,
                                    m_options.m_filteredChainDataOutputAllowedSet);
      if ((m_env.subDisplayFile()          ) &&
          (m_options.m_totallyMute == false)) {
        *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                                << ", prefix = "                << m_options.m_prefix
                                << ": closed sub output file '" << m_options.m_filteredChainDataOutputFileName
                                << "' for filtered chain "      << workingChain.name()
                                << std::endl;
      }
    }

    // Take "unified" care of filtered chain
    if ((m_options.m_filteredChainDataOutputFileName != UQ_MH_SG_FILENAME_FOR_NO_FILE) &&
        (m_options.m_totallyMute == false                                            )) {
      workingChain.unifiedWriteContents(m_options.m_filteredChainDataOutputFileName);
      if ((m_env.subDisplayFile()          ) &&
          (m_options.m_totallyMute == false)) {
        *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                                << ", prefix = "                    << m_options.m_prefix
                                << ": closed unified output file '" << m_options.m_filteredChainDataOutputFileName
                                << "' for filtered chain "          << workingChain.name()
                                << std::endl;
      }
    }

    // Compute statistics
    if (m_options.m_filteredChainComputeStats) {
      workingChain.computeStatistics(*m_options.m_filteredChainStatisticalOptions,
                                     genericOfsVar);
    }
  }

  //****************************************************
  // Close generic output file
  //****************************************************
  if (genericOfsVar) {
    //genericOfsVar->close();
    delete genericOfsVar;
    if ((m_env.subDisplayFile()          ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                              << ", prefix = "                    << m_options.m_prefix
                              << ": closed generic output file '" << m_options.m_dataOutputFileName
                              << "' (chain name is "              << workingChain.name()
                              << ")"
                              << std::endl;
    }
  }

  if ((m_env.subDisplayFile()          ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << std::endl;
  }

  //m_env.syncPrintDebugMsg("Leaving uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()",2,3000000,m_env.fullComm()); // Dangerous to barrier on fullComm ... // KAUST

  if ((m_env.subDisplayFile()          ) &&
      (m_env.displayVerbosity() >= 5   ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "Leaving uqMetropolisHastingsSGClass<P_V,P_M>::generateSequence()"
                            << std::endl;
  }

  return;
}

template <class P_V,class P_M>
void
uqMetropolisHastingsSGClass<P_V,P_M>::readFullChain(
  const std::string&                  inputFileName,
        unsigned int                  chainSize,
  uqBaseVectorSequenceClass<P_V,P_M>& workingChain)
{
  workingChain.unifiedReadContents(inputFileName,chainSize);
  return;
}

template <class P_V,class P_M>
void
uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain(
  const P_V&                          valuesOf1stPosition,
        unsigned int                  chainSize,
  uqBaseVectorSequenceClass<P_V,P_M>& workingChain,
  uqScalarSequenceClass<double>*      workingLogLikelihoodValues,
  uqScalarSequenceClass<double>*      workingLogTargetValues)
{
  //m_env.syncPrintDebugMsg("Entering uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",3,3000000,m_env.fullComm()); // Dangerous to barrier on fullComm ... // KAUST

  if ((m_env.subDisplayFile()          ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "Starting the generation of Markov chain " << workingChain.name()
                            << ", with "                                  << chainSize
                            << " positions..."
                            << std::endl;
  }

  int iRC = UQ_OK_RC;
  struct timeval timevalChain;
  struct timeval timevalCandidate;
  struct timeval timevalTargetD;
  struct timeval timevalMhAlpha;
  struct timeval timevalDrAlpha;
  struct timeval timevalDR;
  struct timeval timevalAM;

  m_positionIdForDebugging = 0;
  m_stageIdForDebugging    = 0;
  m_numRejections          = 0;
  double candidateRunTime = 0;
  double targetDRunTime   = 0;
  double mhAlphaRunTime   = 0;
  double drAlphaRunTime   = 0;
  double drRunTime        = 0;
  double amRunTime        = 0;

  iRC = gettimeofday(&timevalChain, NULL);
  bool outOfTargetSupport = !m_targetPdf.domainSet().contains(valuesOf1stPosition);
  UQ_FATAL_TEST_MACRO(outOfTargetSupport,
                      m_env.fullRank(),
                      "uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",
                      "initial position should not be out of target pdf support");
  if ((m_env.subDisplayFile()          ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                            << ": contents of initial position are:\n";
    *m_env.subDisplayFile() << valuesOf1stPosition; // FIX ME: might need parallelism
    *m_env.subDisplayFile() << std::endl;
  }
  if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalTargetD, NULL);
  double logLikelihood = 0.;
#ifdef QUESO_EXPECTS_LN_LIKELIHOOD_INSTEAD_OF_MINUS_2_LN
  double logTarget =        m_targetPdfSynchronizer->callFunction(&valuesOf1stPosition,NULL,NULL,NULL,NULL,&logLikelihood); // Might demand parallel environment
#else
  double logTarget = -0.5 * m_targetPdfSynchronizer->callFunction(&valuesOf1stPosition,NULL,NULL,NULL,NULL,&logLikelihood); // Might demand parallel environment
#endif
  if (m_options.m_rawChainMeasureRunTimes) targetDRunTime += uqMiscGetEllapsedSeconds(&timevalTargetD);
  //*m_env.subDisplayFile() << "AQUI 001" << std::endl;
  uqMarkovChainPositionDataClass<P_V> currentPositionData(m_env,
                                                          valuesOf1stPosition,
                                                          outOfTargetSupport,
                                                          logLikelihood,
                                                          logTarget);

  P_V gaussianVector(m_vectorSpace.zeroVector());
  P_V tmpVecValues(m_vectorSpace.zeroVector());
  uqMarkovChainPositionDataClass<P_V> currentCandidateData(m_env);

  //****************************************************
  // Begin chain loop from positionId = 1
  //****************************************************
  workingChain.resizeSequence(chainSize); 
  if (workingLogLikelihoodValues) workingLogLikelihoodValues->resizeSequence(chainSize);
  if (workingLogTargetValues    ) workingLogTargetValues->resizeSequence    (chainSize);
  if (true/*m_uniqueChainGenerate*/) m_idsOfUniquePositions.resize(chainSize,0); 
  if (m_options.m_rawChainGenerateExtra) {
    m_logTargets.resize    (chainSize,0.);
    m_alphaQuotients.resize(chainSize,0.);
  }

  unsigned int uniquePos = 0;
  workingChain.setPositionValues(0,currentPositionData.vecValues());
  if (workingLogLikelihoodValues) (*workingLogLikelihoodValues)[0] = currentPositionData.logLikelihood();
  if (workingLogTargetValues    ) (*workingLogTargetValues    )[0] = currentPositionData.logTarget();
  if (true/*m_uniqueChainGenerate*/) m_idsOfUniquePositions[uniquePos++] = 0;
  if (m_options.m_rawChainGenerateExtra) {
    m_logTargets    [0] = currentPositionData.logTarget();
    m_alphaQuotients[0] = 1.;
  }
  //*m_env.subDisplayFile() << "AQUI 002" << std::endl;

  if ((m_env.subDisplayFile()          ) &&
      (m_env.displayVerbosity() >= 10  ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "\n"
                            << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
                            << "\n"
                            << std::endl;
  }

  //m_env.syncPrintDebugMsg("In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain(), right before main loop",3,3000000,m_env.fullComm()); // Dangerous to barrier on fullComm ... // KAUST

  if ((m_env.numSubEnvironments() < (unsigned int) m_env.fullComm().NumProc()) &&
      (m_initialPosition.numOfProcsForStorage() == 1                         ) &&
      (m_env.subRank()                          != 0                         )) {
    // subRank != 0 --> Enter the barrier and wait for processor 0 to decide to call the targetPdf
    double aux = 0.;
    aux = m_targetPdfSynchronizer->callFunction(NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL);
    for (unsigned int positionId = 1; positionId < workingChain.subSequenceSize(); ++positionId) {
      // Multiply by position valules by 'positionId' in order to avoid a constant sequence,
      // which would cause zero variance and eventually OVERFLOW flags raised
      workingChain.setPositionValues(positionId,((double) positionId) * currentPositionData.vecValues());
      m_numRejections++;
    }
  }
  else for (unsigned int positionId = 1; positionId < workingChain.subSequenceSize(); ++positionId) {
    m_positionIdForDebugging = positionId;
    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 10  ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": beginning chain position of id = "   << positionId
                              << ", m_options.m_drMaxNumExtraStages =  " << m_options.m_drMaxNumExtraStages
                              << std::endl;
    }
    unsigned int stageId  = 0;
    m_stageIdForDebugging = stageId;

    m_tk->clearPreComputingPositions();

    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 5   ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": about to set TK pre computing position of local id " << 0
                              << ", values = " << currentPositionData.vecValues()
                              << std::endl;
    }
    bool validPreComputingPosition = m_tk->setPreComputingPosition(currentPositionData.vecValues(),0);
    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 5   ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": returned from setting TK pre computing position of local id " << 0
                              << ", values = " << currentPositionData.vecValues()
                              << ", valid = "  << validPreComputingPosition
                              << std::endl;
    }
    UQ_FATAL_TEST_MACRO(validPreComputingPosition == false,
                        m_env.fullRank(),
                        "uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",
                        "initial position should not be an invalid pre computing position");

    //****************************************************
    // Loop: generate new position
    //****************************************************
    bool keepGeneratingCandidates = true;
    while (keepGeneratingCandidates) {
      if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalCandidate, NULL);
      m_tk->rv(0).realizer().realization(tmpVecValues);
      if (m_options.m_rawChainMeasureRunTimes) candidateRunTime += uqMiscGetEllapsedSeconds(&timevalCandidate);

      outOfTargetSupport = !m_targetPdf.domainSet().contains(tmpVecValues);

      bool displayDetail = (m_env.displayVerbosity() >= 10/*99*/) || m_options.m_displayCandidates;
      if ((m_env.subDisplayFile()          ) &&
          (displayDetail                   ) &&
          (m_options.m_totallyMute == false)) {
        *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                                << ": for chain position of id = " << positionId
                                << ", candidate = "                << tmpVecValues // FIX ME: might need parallelism
                                << ", outOfTargetSupport = "       << outOfTargetSupport
                                << std::endl;
      }

      if (m_options.m_putOutOfBoundsInChain) keepGeneratingCandidates = false;
      else                                   keepGeneratingCandidates = outOfTargetSupport;
    }

    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 5   ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": about to set TK pre computing position of local id " << stageId+1
                              << ", values = " << tmpVecValues
                              << std::endl;
    }
    validPreComputingPosition = m_tk->setPreComputingPosition(tmpVecValues,stageId+1);
    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 5   ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": returned from setting TK pre computing position of local id " << stageId+1
                              << ", values = " << tmpVecValues
                              << ", valid = "  << validPreComputingPosition
                              << std::endl;
    }

    if (outOfTargetSupport) {
      m_numOutOfTargetSupport++;
      logLikelihood = -INFINITY;
      logTarget     = -INFINITY;
    }
    else {
      if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalTargetD, NULL);
#ifdef QUESO_EXPECTS_LN_LIKELIHOOD_INSTEAD_OF_MINUS_2_LN
      logTarget =        m_targetPdfSynchronizer->callFunction(&tmpVecValues,NULL,NULL,NULL,NULL,&logLikelihood); // Might demand parallel environment
#else
      logTarget = -0.5 * m_targetPdfSynchronizer->callFunction(&tmpVecValues,NULL,NULL,NULL,NULL,&logLikelihood); // Might demand parallel environment
#endif
      if (m_options.m_rawChainMeasureRunTimes) targetDRunTime += uqMiscGetEllapsedSeconds(&timevalTargetD);
    }
    currentCandidateData.set(tmpVecValues,
                             outOfTargetSupport,
                             logLikelihood,
                             logTarget);

    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 10  ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "\n"
                              << "\n-----------------------------------------------------------\n"
                              << "\n"
                              << std::endl;
    }
    bool accept = false;
    double alphaFirstCandidate = 0.;
    if (outOfTargetSupport) {
      if (m_options.m_rawChainGenerateExtra) {
        m_alphaQuotients[positionId] = 0.;
      }
    }
    else {
      if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalMhAlpha, NULL);
      if (m_options.m_rawChainGenerateExtra) {
        alphaFirstCandidate = this->alpha(currentPositionData,currentCandidateData,0,1,&m_alphaQuotients[positionId]);
      }
      else {
        alphaFirstCandidate = this->alpha(currentPositionData,currentCandidateData,0,1,NULL);
      }
      if (m_options.m_rawChainMeasureRunTimes) mhAlphaRunTime += uqMiscGetEllapsedSeconds(&timevalMhAlpha);
      if ((m_env.subDisplayFile()          ) &&
          (m_env.displayVerbosity() >= 10  ) &&
          (m_options.m_totallyMute == false)) {
        *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                                << ": for chain position of id = " << positionId
                                << std::endl;
      }
      accept = acceptAlpha(alphaFirstCandidate);
    }

    bool displayDetail = (m_env.displayVerbosity() >= 10/*99*/) || m_options.m_displayCandidates;
    if ((m_env.subDisplayFile()          ) &&
        (displayDetail                   ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": for chain position of id = " << positionId
                              << ", outOfTargetSupport = "       << outOfTargetSupport
                              << ", alpha = "                    << alphaFirstCandidate
                              << ", accept = "                   << accept
                              << ", currentCandidateData.vecValues() = ";
      *m_env.subDisplayFile() << currentCandidateData.vecValues(); // FIX ME: might need parallelism
      *m_env.subDisplayFile() << "\n"
                              << "\n curLogTarget  = "           << currentPositionData.logTarget()
                              << "\n"
                              << "\n canLogTarget  = "           << currentCandidateData.logTarget()
                              << "\n"
                              << std::endl;
    }
    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 10  ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "\n"
                              << "\n-----------------------------------------------------------\n"
                              << "\n"
                              << std::endl;
    }

    //****************************************************
    // Loop: delayed rejection
    //****************************************************
    std::vector<uqMarkovChainPositionDataClass<P_V>*> drPositionsData(stageId+2,NULL);
    std::vector<unsigned int> tkStageIds (stageId+2,0);
    if ((accept == false) && (outOfTargetSupport == false) && (m_options.m_drMaxNumExtraStages > 0)) {
      if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalDR, NULL);

      drPositionsData[0] = new uqMarkovChainPositionDataClass<P_V>(currentPositionData );
      drPositionsData[1] = new uqMarkovChainPositionDataClass<P_V>(currentCandidateData);

      tkStageIds[0]  = 0;
      tkStageIds[1]  = 1;

      while ((validPreComputingPosition == true        ) && 
             (accept                    == false       ) &&
             (stageId < m_options.m_drMaxNumExtraStages)) {
        if ((m_env.subDisplayFile()          ) &&
            (m_env.displayVerbosity() >= 10  ) &&
            (m_options.m_totallyMute == false)) {
          *m_env.subDisplayFile() << "\n"
                                  << "\n+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-"
                                  << "\n"
                                  << std::endl;
        }
        stageId++;
        m_stageIdForDebugging = stageId;
        if ((m_env.subDisplayFile()          ) &&
            (m_env.displayVerbosity() >= 10  ) &&
            (m_options.m_totallyMute == false)) {
          *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                                  << ": for chain position of id = " << positionId
                                  << ", beginning stageId = "        << stageId
                                  << std::endl;
        }

        keepGeneratingCandidates = true;
        while (keepGeneratingCandidates) {
          if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalCandidate, NULL);
          m_tk->rv(tkStageIds).realizer().realization(tmpVecValues);
          if (m_options.m_rawChainMeasureRunTimes) candidateRunTime += uqMiscGetEllapsedSeconds(&timevalCandidate);

          outOfTargetSupport = !m_targetPdf.domainSet().contains(tmpVecValues);

          if (m_options.m_putOutOfBoundsInChain) keepGeneratingCandidates = false;
          else                                   keepGeneratingCandidates = outOfTargetSupport;
        }

        if ((m_env.subDisplayFile()          ) &&
            (m_env.displayVerbosity() >= 5   ) &&
            (m_options.m_totallyMute == false)) {
          *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                                  << ": about to set TK pre computing position of local id " << stageId+1
                                  << ", values = " << tmpVecValues
                                  << std::endl;
        }
        validPreComputingPosition = m_tk->setPreComputingPosition(tmpVecValues,stageId+1);
        if ((m_env.subDisplayFile()          ) &&
            (m_env.displayVerbosity() >= 5   ) &&
            (m_options.m_totallyMute == false)) {
          *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                                  << ": returned from setting TK pre computing position of local id " << stageId+1
                                  << ", values = " << tmpVecValues
                                  << ", valid = "  << validPreComputingPosition
                                  << std::endl;
        }

        if (outOfTargetSupport) {
          logLikelihood = -INFINITY;
          logTarget     = -INFINITY;
        }
        else {
          if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalTargetD, NULL);
#ifdef QUESO_EXPECTS_LN_LIKELIHOOD_INSTEAD_OF_MINUS_2_LN
          logTarget =        m_targetPdfSynchronizer->callFunction(&tmpVecValues,NULL,NULL,NULL,NULL,&logLikelihood); // Might demand parallel environment
#else
          logTarget = -0.5 * m_targetPdfSynchronizer->callFunction(&tmpVecValues,NULL,NULL,NULL,NULL,&logLikelihood); // Might demand parallel environment
#endif
          if (m_options.m_rawChainMeasureRunTimes) targetDRunTime += uqMiscGetEllapsedSeconds(&timevalTargetD);
        }
        currentCandidateData.set(tmpVecValues,
                                 outOfTargetSupport,
                                 logLikelihood,
                                 logTarget);

        drPositionsData.push_back(new uqMarkovChainPositionDataClass<P_V>(currentCandidateData));
        tkStageIds.push_back     (stageId+1);

        double alphaDR = 0.;
        if (outOfTargetSupport == false) {
          if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalDrAlpha, NULL);
          alphaDR = this->alpha(drPositionsData,tkStageIds);
          if (m_options.m_rawChainMeasureRunTimes) drAlphaRunTime += uqMiscGetEllapsedSeconds(&timevalDrAlpha);
          accept = acceptAlpha(alphaDR);
        }

        displayDetail = (m_env.displayVerbosity() >= 10/*99*/) || m_options.m_displayCandidates;
        if ((m_env.subDisplayFile()          ) &&
            (displayDetail                   ) &&
            (m_options.m_totallyMute == false)) {
          *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                                  << ": for chain position of id = " << positionId
                                  << " and stageId = "               << stageId
                                  << ", outOfTargetSupport = "       << outOfTargetSupport
                                  << ", alpha = "                    << alphaDR
                                  << ", accept = "                   << accept
                                  << ", currentCandidateData.vecValues() = ";
          *m_env.subDisplayFile() << currentCandidateData.vecValues(); // FIX ME: might need parallelism
          *m_env.subDisplayFile() << std::endl;
        }
      } // while

      if (m_options.m_rawChainMeasureRunTimes) drRunTime += uqMiscGetEllapsedSeconds(&timevalDR);
    } // end of 'delayed rejection' logic

    for (unsigned int i = 0; i < drPositionsData.size(); ++i) {
      if (drPositionsData[i]) delete drPositionsData[i];
    }

    //****************************************************
    // Loop: update chain
    //****************************************************
    if (accept) {
      workingChain.setPositionValues(positionId,currentCandidateData.vecValues());
      if (true/*m_uniqueChainGenerate*/) m_idsOfUniquePositions[uniquePos++] = positionId;
      currentPositionData = currentCandidateData;
    }
    else {
      workingChain.setPositionValues(positionId,currentPositionData.vecValues());
      m_numRejections++;
    }

    if (workingLogLikelihoodValues) (*workingLogLikelihoodValues)[positionId] = currentPositionData.logLikelihood();
    if (workingLogTargetValues    ) (*workingLogTargetValues    )[positionId] = currentPositionData.logTarget();

    if (m_options.m_rawChainGenerateExtra) {
      m_logTargets[positionId] = currentPositionData.logTarget();
    }

    //****************************************************
    // Loop: adaptive Metropolis (adaptation of covariance matrix)
    //****************************************************
    if ((m_options.m_tkUseLocalHessian ==    false) && // IMPORTANT
        (m_options.m_amInitialNonAdaptInterval > 0) &&
        (m_options.m_amAdaptInterval           > 0)) {
      if (m_options.m_rawChainMeasureRunTimes) iRC = gettimeofday(&timevalAM, NULL);

      // Now might be the moment to adapt
      unsigned int idOfFirstPositionInSubChain = 0;
      uqSequenceOfVectorsClass<P_V,P_M> partialChain(m_vectorSpace,0,m_options.m_prefix+"partialChain");

      // Check if now is indeed the moment to adapt
      if (positionId < m_options.m_amInitialNonAdaptInterval) {
        // Do nothing
      }
      else if (positionId == m_options.m_amInitialNonAdaptInterval) {
        idOfFirstPositionInSubChain = 0;
        partialChain.resizeSequence(m_options.m_amInitialNonAdaptInterval+1);
        m_lastMean             = m_vectorSpace.newVector();
        m_lastAdaptedCovMatrix = m_vectorSpace.newMatrix();
      }
      else {
        unsigned int interval = positionId - m_options.m_amInitialNonAdaptInterval;
        if ((interval % m_options.m_amAdaptInterval) == 0) {
          idOfFirstPositionInSubChain = positionId - m_options.m_amAdaptInterval;
          partialChain.resizeSequence(m_options.m_amAdaptInterval);
        }
      }

      // If now is indeed the moment to adapt, then do it!
      if (partialChain.subSequenceSize() > 0) {
        P_V transporterVec(m_vectorSpace.zeroVector());
        for (unsigned int i = 0; i < partialChain.subSequenceSize(); ++i) {
          workingChain.getPositionValues(idOfFirstPositionInSubChain+i,transporterVec);
          partialChain.setPositionValues(i,transporterVec);
        }
        updateAdaptedCovMatrix(partialChain,
                               idOfFirstPositionInSubChain,
                               m_lastChainSize,
                              *m_lastMean,
                              *m_lastAdaptedCovMatrix);

        bool tmpCholIsPositiveDefinite = false;
        P_M tmpChol(*m_lastAdaptedCovMatrix);
        P_M attemptedMatrix(tmpChol);
        //if (m_env.subDisplayFile()) {
        //  *m_env.subDisplayFile() << "DRAM"
        //                          << ", positionId = "  << positionId
        //                          << ": 'am' calling first tmpChol.chol()"
        //                          << std::endl;
        //}
        iRC = tmpChol.chol();
        //if (m_env.subDisplayFile()) {
        //  *m_env.subDisplayFile() << "DRAM"
        //                          << ", positionId = "  << positionId
        //                          << ": 'am' got first tmpChol.chol() with iRC = " << iRC
        //                          << std::endl;
        //}
        if (iRC) {
          UQ_FATAL_TEST_MACRO(iRC != UQ_MATRIX_IS_NOT_POS_DEFINITE_RC,
                              m_env.fullRank(),
                              "uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",
                              "invalid iRC returned from first chol()");
          // Matrix is not positive definite
          P_M* tmpDiag = m_vectorSpace.newDiagMatrix(m_options.m_amEpsilon);
          tmpChol = *m_lastAdaptedCovMatrix + *tmpDiag;
          attemptedMatrix = tmpChol;
          delete tmpDiag;
          //if (m_env.subDisplayFile()) {
          //  *m_env.subDisplayFile() << "DRAM"
          //                          << ", positionId = "  << positionId
          //                          << ": 'am' calling second tmpChol.chol()"
          //                          << std::endl;
          //}
          iRC = tmpChol.chol();
          //if (m_env.subDisplayFile()) {
          //  *m_env.subDisplayFile() << "DRAM"
          //                          << ", positionId = "  << positionId
          //                          << ": 'am' got second tmpChol.chol() with iRC = " << iRC
          //                          << std::endl;
          //}
          if (iRC) {
            UQ_FATAL_TEST_MACRO(iRC != UQ_MATRIX_IS_NOT_POS_DEFINITE_RC,
                                m_env.fullRank(),
                                "uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",
                                "invalid iRC returned from second chol()");
            // Do nothing
          }
          else {
            tmpCholIsPositiveDefinite = true;
          }
        }
        else {
          tmpCholIsPositiveDefinite = true;
        }
        if (tmpCholIsPositiveDefinite) {
          uqScaledCovMatrixTKGroupClass<P_V,P_M>* tempTK = dynamic_cast<uqScaledCovMatrixTKGroupClass<P_V,P_M>* >(m_tk);
          tempTK->updateLawCovMatrix(m_options.m_amEta*attemptedMatrix);

#ifdef UQ_DRAM_MCG_REQUIRES_INVERTED_COV_MATRICES
          UQ_FATAL_RC_MACRO(UQ_INCOMPLETE_IMPLEMENTATION_RC,
                            m_env.fullRank(),
                            "uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",
                            "need to code the update of m_upperCholProposalPrecMatrices");
#endif
        }

        //for (unsigned int i = 0; i < partialChain.subSequenceSize(); ++i) {
        //  if (partialChain[i]) delete partialChain[i];
        //}
      }

      if (m_options.m_rawChainMeasureRunTimes) amRunTime += uqMiscGetEllapsedSeconds(&timevalAM);
    } // End of 'adaptive Metropolis' logic

    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 10  ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "In uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()"
                              << ": finishing chain position of id = " << positionId
                              << std::endl;
    }

    if ((m_options.m_rawChainDisplayPeriod                     > 0) && 
        (((positionId+1) % m_options.m_rawChainDisplayPeriod) == 0)) {
      if ((m_env.subDisplayFile()          ) &&
          (m_options.m_totallyMute == false)) {
        *m_env.subDisplayFile() << "Finished generating " << positionId+1
                                << " positions"
                                << std::endl;
      }
    }

    if ((m_env.subDisplayFile()          ) &&
        (m_env.displayVerbosity() >= 10  ) &&
        (m_options.m_totallyMute == false)) {
      *m_env.subDisplayFile() << "\n"
                              << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
                              << "\n"
                              << std::endl;
    }
  } // end chain loop

  if ((m_env.numSubEnvironments() < (unsigned int) m_env.fullComm().NumProc()) &&
      (m_initialPosition.numOfProcsForStorage() == 1                         ) &&
      (m_env.subRank()                          == 0                         )) {
    // subRank == 0 --> Tell all other processors to exit barrier now that the chain has been fully generated
    double aux = 0.;
    aux = m_targetPdfSynchronizer->callFunction(NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL);
  }

  //****************************************************
  // Print basic information about the chain
  //****************************************************
  m_rawChainRunTime += uqMiscGetEllapsedSeconds(&timevalChain);
  if ((m_env.subDisplayFile()          ) &&
      (m_options.m_totallyMute == false)) {
    *m_env.subDisplayFile() << "Finished the generation of Markov chain " << workingChain.name()
                            << ", with sub "                               << workingChain.subSequenceSize()
                            << " positions";
    *m_env.subDisplayFile() << "\nSome information about this chain:"
                            << "\n  Chain run time       = " << m_rawChainRunTime
                            << " seconds";
    if (m_options.m_rawChainMeasureRunTimes) {
      *m_env.subDisplayFile() << "\n\n Breaking of the chain run time:\n";
      *m_env.subDisplayFile() << "\n  Candidate run time   = " << candidateRunTime
                              << " seconds ("                   << 100.*candidateRunTime/m_rawChainRunTime
                              << "%)";
      *m_env.subDisplayFile() << "\n  Target d. run time   = " << targetDRunTime
                              << " seconds ("                   << 100.*targetDRunTime/m_rawChainRunTime
                              << "%)";
      *m_env.subDisplayFile() << "\n  Mh alpha run time    = " << mhAlphaRunTime
                              << " seconds ("                   << 100.*mhAlphaRunTime/m_rawChainRunTime
                              << "%)";
      *m_env.subDisplayFile() << "\n  Dr alpha run time    = " << drAlphaRunTime
                              << " seconds ("                   << 100.*drAlphaRunTime/m_rawChainRunTime
                              << "%)";
      *m_env.subDisplayFile() << "\n----------------------   --------------";
      double sumRunTime = candidateRunTime + targetDRunTime + mhAlphaRunTime + drAlphaRunTime;
      *m_env.subDisplayFile() << "\n  Sum                  = " << sumRunTime
                              << " seconds ("                   << 100.*sumRunTime/m_rawChainRunTime
                              << "%)";
      *m_env.subDisplayFile() << "\n\n Other run times:";
      *m_env.subDisplayFile() << "\n  DR run time          = " << drRunTime
                              << " seconds ("                   << 100.*drRunTime/m_rawChainRunTime
                              << "%)";
      *m_env.subDisplayFile() << "\n  AM run time          = " << amRunTime
                              << " seconds ("                   << 100.*amRunTime/m_rawChainRunTime
                              << "%)";
    }
    *m_env.subDisplayFile() << "\n  Rejection percentage = "   << 100. * (double) m_numRejections/(double) workingChain.subSequenceSize()
                            << " %";
    *m_env.subDisplayFile() << "\n  Out of target support percentage = " << 100. * (double) m_numOutOfTargetSupport/(double) workingChain.subSequenceSize()
                            << " %";
    *m_env.subDisplayFile() << std::endl;
  }

  //****************************************************
  // Release memory before leaving routine
  //****************************************************
  //m_env.syncPrintDebugMsg("Leaving uqMetropolisHastingsSGClass<P_V,P_M>::generateFullChain()",3,3000000,m_env.fullComm()); // Dangerous to barrier on fullComm ... // KAUST

  return;
}

template <class P_V,class P_M>
void
uqMetropolisHastingsSGClass<P_V,P_M>::updateAdaptedCovMatrix(
  const uqBaseVectorSequenceClass<P_V,P_M>& partialChain,
  unsigned int                              idOfFirstPositionInSubChain,
  double&                                   lastChainSize,
  P_V&                                      lastMean,
  P_M&                                      lastAdaptedCovMatrix)
{
  double doubleSubChainSize = (double) partialChain.subSequenceSize();
  if (lastChainSize == 0) {
    UQ_FATAL_TEST_MACRO(partialChain.subSequenceSize() < 2,
                        m_env.fullRank(),
                        "uqMetropolisHastingsSGClass<P_V,P_M>::updateAdaptedCovMatrix()",
                        "'partialChain.subSequenceSize()' should be >= 2");

    partialChain.subMean(0,partialChain.subSequenceSize(),lastMean);

    P_V tmpVec(m_vectorSpace.zeroVector());
    lastAdaptedCovMatrix = -doubleSubChainSize * matrixProduct(lastMean,lastMean);
    for (unsigned int i = 0; i < partialChain.subSequenceSize(); ++i) {
      partialChain.getPositionValues(i,tmpVec);
      lastAdaptedCovMatrix += matrixProduct(tmpVec,tmpVec);
    }
    lastAdaptedCovMatrix /= (doubleSubChainSize - 1.); // That is why partialChain size must be >= 2
  }
  else {
    UQ_FATAL_TEST_MACRO(partialChain.subSequenceSize() < 1,
                        m_env.fullRank(),
                        "uqMetropolisHastingsSGClass<P_V,P_M>::updateAdaptedCovMatrix()",
                        "'partialChain.subSequenceSize()' should be >= 1");

    UQ_FATAL_TEST_MACRO(idOfFirstPositionInSubChain < 1,
                        m_env.fullRank(),
                        "uqMetropolisHastingsSGClass<P_V,P_M>::updateAdaptedCovMatrix()",
                        "'idOfFirstPositionInSubChain' should be >= 1");

    P_V tmpVec (m_vectorSpace.zeroVector());
    P_V diffVec(m_vectorSpace.zeroVector());
    for (unsigned int i = 0; i < partialChain.subSequenceSize(); ++i) {
      double doubleCurrentId  = (double) (idOfFirstPositionInSubChain+i);
      partialChain.getPositionValues(i,tmpVec);
      diffVec = tmpVec - lastMean;

      double ratio1         = (1. - 1./doubleCurrentId); // That is why idOfFirstPositionInSubChain must be >= 1
      double ratio2         = (1./(1.+doubleCurrentId));
      lastAdaptedCovMatrix  = ratio1 * lastAdaptedCovMatrix + ratio2 * matrixProduct(diffVec,diffVec);
      lastMean             += ratio2 * diffVec;
    } 
  }
  lastChainSize += doubleSubChainSize;

  return;
}
#endif // __UQ_MH_SG2_H__
