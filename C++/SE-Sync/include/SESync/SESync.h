/** This file provides a convenient functional interface to the SE-Sync
 * algorithm.
 *
 * Copyright (C) 2016 - 2022 by David M. Rosen (dmrosen@mit.edu)
 */

#pragma once

#include <vector>

#include <Eigen/Dense>

#include "SESync/RelativePoseMeasurement.h"
#include "SESync/SESyncProblem.h"
#include "SESync/SESync_types.h"

namespace SESync {

/** This struct contains the various parameters that control the SESync
 * algorithm */
struct SESyncOpts {
  /// OPTIMIZATION STOPPING CRITERIA

  /** Stopping tolerance for the norm of the Riemannian gradient */
  Scalar grad_norm_tol = 1e-2;

  /** Stopping tolerance for the norm of the preconditioned Riemannian gradient
   */
  Scalar preconditioned_grad_norm_tol = 1e-4;

  /** Stopping criterion based upon the relative decrease in function value
   * between accepted iterations */
  Scalar rel_func_decrease_tol = 1e-6;

  /** Stopping criterion based upon the norm of an accepted update step */
  Scalar stepsize_tol = 1e-3;

  /** Maximum permitted number of (outer) iterations of the Riemannian
   * trust-region method when solving each instance of Problem 9 */
  size_t max_iterations = 1000;

  /** Maximum number of inner (truncated conjugate-gradient) iterations to
   * perform per outer iteration */
  size_t max_tCG_iterations = 10000;

  /** Maximum elapsed computation time (in seconds) */
  double max_computation_time = 1800;

  /// These next two parameters define the stopping criteria for the truncated
  /// preconditioned conjugate-gradient solver running in the inner loop --
  /// they control the tradeoff between the quality of the returned
  /// trust-region update step (as a minimizer of the local quadratic model
  /// computed at each iterate) and the computational expense needed to generate
  /// that update step.  You probably don't need to modify these unless you
  /// really know what you're doing.

  /** Gradient tolerance for the truncated preconditioned conjugate gradient
   * solver: stop if ||g|| < kappa * ||g_0||.  This parameter should be in the
   * range (0,1). */
  Scalar STPCG_kappa = .1;

  /** Gradient tolerance based upon a fractional-power reduction in the norm of
   * the gradient: stop if ||g|| < ||kappa||^{1+ theta}.  This value should be
   * positive, and controls the asymptotic convergence rate of the
   * truncated-Newton trust-region solver: specifically, for theta > 0, the TNT
   * algorithm converges q-superlinearly with order (1+theta). */
  Scalar STPCG_theta = .5;

  /** An optional user-supplied function that can be used to instrument/monitor
   * the performance of the internal Riemannian truncated-Newton trust-region
   * optimization algorithm as it runs. */
  std::optional<SESyncTNTUserFunction> user_function;

  /// SE-SYNC PARAMETERS

  /** The specific formulation of the SE-Sync problem to solve */
  Formulation formulation = Formulation::Simplified;

  /** The initial level of the Riemannian Staircase */
  size_t r0 = 5;

  /** The maximum level of the Riemannian Staircase to explore */
  size_t rmax = 10;

  /** Tolerance for accepting the minimum eigenvalue of the
   * certificate matrix as numerically nonnegative; this should be a small
   * positive value e.g. 10^-3 */
  Scalar min_eig_num_tol = 1e-3;

  /** Block size to use in LOBPCG when computing a minimum eigenpair of the
   * certificate matrix */
  size_t LOBPCG_block_size = 4;

  /// The next parameters control the sparsity of the incomplete symmetric
  /// indefinite factorization-based preconditioner used in conjunction with
  /// LOBPCG: 'max_fill_factor' and 'drop_tol' are parameters controlling the
  /// each column of the inexact sparse triangular *factor L is guanteed to have
  /// at most max_fill_factor *(nnz(A) / dim(A)) nonzero elements, and any
  /// elements l in L_k(the kth column of L) satisfying |l| <= drop_tol *
  /// |L_k|_1 will be set to 0.
  Scalar LOBPCG_max_fill_factor = 3;
  Scalar LOBPCG_drop_tol = 1e-3;

  /** The maximum number of LOBPCG iterations to permit for the
   * minimum-eigenpair computation */
  size_t LOBPCG_max_iterations = 100;

  /** Whether to use the Cholesky or QR factorization when
   * computing the orthogonal projection */
  ProjectionFactorization projection_factorization =
      ProjectionFactorization::Cholesky;

  /** The preconditioning strategy to use in the Riemannian trust-region
   * algorithm*/
  Preconditioner preconditioner = Preconditioner::RegularizedCholesky;

  /** Maximum admissible condition number for the regularized Cholesky
   * preconditioner */
  Scalar reg_Cholesky_precon_max_condition_number = 1e6;

  /** The initialization method to use for constructing an initial iterate Y0,
   * if none was provided */
  Initialization initialization = Initialization::Chordal;

  /** Whether to print output as the algorithm runs */
  bool verbose = false;

  /** If this value is true, the SE-Sync algorithm will log and return the
   * entire sequence of iterates generated by the Riemannian Staircase */
  bool log_iterates = false;

  /** The number of threads to use for parallelization (assuming that SE-Sync is
   * built using a compiler that supports OpenMP */
  size_t num_threads = 1;
};

/** These enumerations describe the termination status of the SE-Sync algorithm
 */
enum SESyncStatus {
  /** The algorithm converged to a certified global optimum */
  GlobalOpt,

  /** The algorithm converged to a saddle point, but the backtracking line
   * search was unable to escape it */
  SaddlePoint,

  /** The algorithm converged to a first-order critical point, but the
   * minimum-eigenpair computation did not converge to sufficient precision to
   * enable its characterization */
  EigImprecision,

  /** The algorithm exhausted the maximum number of iterations of the Riemannian
   * Staircase before finding an optimal solution */
  MaxRank,

  /** The algorithm exhausted the allotted total computation time before finding
   * an optimal solution */
  ElapsedTime
};

/** This struct contains the output of the SESync algorithm */
struct SESyncResult {

  /** An estimate of a global minimizer Yopt of the rank-restricted dual
   * semidefinite relaxation Problem 9 in the SE-Sync tech report.  The
   * corresponding solution of Problem 7 is Z = Y^T Y */
  Matrix Yopt;

  /** The value of the objective F(Y^T Y) = F(Z) attained by the Yopt */
  Scalar SDPval;

  /** The norm of the Riemannian gradient at Yopt */
  Scalar gradnorm;

  /** The Lagrange multiplier matrix Lambda corresponding to Yopt, computed
   * according to eq. (119) in the SE-Sync tech report.  If Z = Y^T Y is an
   * exact solution for the dual semidefinite relaxation Problem 7, then Lambda
   * is the solution to the primal Lagrangian relaxation Problem 6. */
  SparseMatrix Lambda;

  /** The trace of Lambda; this is the value of Lambda under the objective of
   * the (primal) semidefinite relaxation Problem 6. */
  Scalar trLambda;

  /** The duality gap between the estimates for the primal and dual solutions
   * Lambda and Z = Y^T Y of Problems 7 and 6, respectively; it is given by:
   *
   * duality_gap := F(Y^T Y) - tr(Lambda)
   *
   */
  Scalar duality_gap;

  /** The objective value of the rounded solution xhat in SE(d)^n */
  Scalar Fxhat;

  /** The rounded solution xhat = [t | R] in SE(d)^n */
  Matrix xhat;

  /** Upper bound on the global suboptimality of the recovered estimates
   * xhat; this is equal to F(xhat) - tr(Lambda) */
  Scalar suboptimality_bound;

  /** The total elapsed computation time for the SE-Sync algorithm */
  double total_computation_time;

  /** The elapsed computation time used to compute the initialization for the
   * Riemannian Staircase */
  double initialization_time;

  /** A vector containing the sequence of function values obtained during the
   * optimization at each level of the Riemannian Staircase */
  std::vector<std::vector<Scalar>> function_values;

  /** A vector containing the sequence of norms of the Riemannian gradients
   * obtained during the optimization at each level of the Riemannian Staircase
   */
  std::vector<std::vector<Scalar>> gradient_norms;

  /** A vector containing the sequence of norms of the preconditioned Riemannian
   * gradients obtained during the optimization at each level of the Riemannian
   * Staircase */
  std::vector<std::vector<Scalar>> preconditioned_gradient_norms;

  /** A vector containing the sequence of (# Hessian-vector product operations)
   * carried out during the optimization at each level of the Riemannian
   * Staircase */
  std::vector<std::vector<size_t>> Hessian_vector_products;

  /** A vector containing the sequence of elapsed times in the optimization at
   * each level of the Riemannian Staircase at which the corresponding function
   * values and gradients were obtained */
  std::vector<std::vector<double>> elapsed_optimization_times;

  /** A vector containing the sequence of curvatures theta := x'*S*x of the
   * certificate matrices S along the computed escape directions x from
   * suboptimal critical points at each level of the Riemannian Staircase
   */
  std::vector<Scalar> escape_direction_curvatures;

  /** A vector containing the number of LOBPCG iterations performed for the
   * minimum-eigenpair computation at each level of the Riemannian Staircase */
  std::vector<size_t> LOBPCG_iters;

  /** A vector containing the elapsed time needed to perform solution
   * verification at each level of the Riemannian Staircase */
  std::vector<double> verification_times;

  /** If log_iterates = true, this will contain the sequence of iterates
   * generated by the truncated-Newton trust-region method at each
   * level of the Riemannian Staircase */
  std::vector<std::vector<Matrix>> iterates;

  /** The termination status of the SE-Sync algorithm */
  SESyncStatus status;
};

/** Given an SESyncProblem instance, this function performs synchronization */
SESyncResult SESync(SESyncProblem &problem,
                    const SESyncOpts &options = SESyncOpts(),
                    const Matrix &Y0 = Matrix());

/** Given a vector of relative pose measurements specifying a special Euclidean
 * synchronization problem, performs synchronization using the SESync algorithm
 */
SESyncResult SESync(const measurements_t &measurements,
                    const SESyncOpts &options = SESyncOpts(),
                    const Matrix &Y0 = Matrix());

/** Helper function: used in the Riemannian Staircase to escape from a saddle
 *  point.  Here:
 *
 * - problem is the specific special Euclidean synchronization problem we are
 *   attempting to solve
 * - Y is the critical point (saddle point) obtained at the current level of the
 *   Riemannian Staircase
 * - lambda_min is the (negative) minimum eigenvalue of the matrix Q - Lambda
 * - v_min is the eigenvector corresponding to lambda_min
 * - gradient_tolerance is a *lower bound* on the norm of the Riemannian
 *   gradient grad F(Yplus) in order to accept a candidate point Yplus as a
 *   valid solution
 *
 * Post-condition:  This function returns a Boolean value indicating whether it
 * was able to successfully escape from the saddle point, meaning it found a
 * point Yplus satisfying the following two criteria:
 *
 *  (1)  F(Yplus) < F(Y), and
 *  (2)  || grad F(Yplus) || > gradient_tolerance
 *
 * Condition (2) above is necessary to ensure that the optimization initialized
 * at the next level of the Riemannian Staircase does not immediately terminate
 * due to the gradient stopping tolerance being satisfied.
 *
 * Precondition: the relaxation rank r of 'problem' must be 1 greater than the
 * number of rows of Y (i.e., the relaxation rank of 'problem' must already be
 * set for the *next* level of the Riemannian Staircase when this function is
 * called.
 *
 * Postcondition: If this function returns true, then upon termination Yplus
 * contains the point at which to initialize the optimization at the next level
 * of the Riemannian Staircase
 */
bool escape_saddle(const SESyncProblem &problem, const Matrix &Y, Scalar theta,
                   const Vector &v, Scalar gradient_tolerance,
                   Scalar preconditioned_gradient_tolerance, Matrix &Yplus);

} // namespace SESync
