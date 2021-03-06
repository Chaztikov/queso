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
 * Brief description of this file:
 *
 * This is an example of how to define and solve a statistical inverse problem
 * using QUESO classes and algorithms. The code itself is in the templated
 * routine 'uqAppl(*env)'. This routine is called right after the initialization
 * of the MPI environment and of the QUESO environment and is available in
 * file 'exStatisticalInverseProblem_appl.h'
 *
 *--------------------------------------------------------------------------
 *-------------------------------------------------------------------------- */

#include <exStatisticalInverseProblem_appl.h>
#include <queso/GslMatrix.h>

int main(int argc, char* argv[])
{
  //************************************************
  // Initialize environments
  //************************************************
  MPI_Init(&argc,&argv);
  UQ_FATAL_TEST_MACRO(argc != 2,
                      QUESO::UQ_UNAVAILABLE_RANK,
                      "main()",
                      "input file must be specified in command line as argv[1], just after executable argv[0]");
  QUESO::FullEnvironment* env = new QUESO::FullEnvironment(MPI_COMM_WORLD,argv[1],"",NULL);
  //std::cout << "proc " << env->fullRank() << ", HERE main 000" << std::endl;
  //env->fullComm().Barrier();
  //std::cout << "proc " << env->fullRank() << ", HERE main 001" << std::endl;

  //************************************************
  // Call application
  //************************************************
  uqAppl<QUESO::GslVector, // type for parameter vectors
         QUESO::GslMatrix  // type for parameter matrices
        >(*env);

  //************************************************
  // Finalize environments
  //************************************************
  delete env;
  MPI_Finalize();
  return 0;
}
