/*
      minimum free energy
      RNA secondary structure with
      basepair distance d to reference structure prediction

*/
#ifndef __VIENNA_RNA_PACKAGE_TWO_D_PF_FOLD_H__
#define __VIENNA_RNA_PACKAGE_TWO_D_PF_FOLD_H__

#include <ViennaRNA/data_structures.h>
#include <ViennaRNA/2Dfold.h>

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#else
#define DEPRECATED(func) func
#endif

/**
 *  \addtogroup kl_neighborhood_pf
 *  \brief Compute the partition function and stochastically sample secondary structures for a partitioning of
 *  the secondary structure space according to the base pair distance to two fixed reference structures
 *  @{
 *
 *  \file 2Dpfold.h
 */


/**
 *  \brief Solution element returned from TwoDpfoldList
 *
 *  This element contains the partition function for the appropriate
 *  kappa (k), lambda (l) neighborhood
 *  The datastructure contains two integer attributes 'k' and 'l'
 *  as well as an attribute 'q' of type #FLT_OR_DBL
 *
 *  A value of #INF in k denotes the end of a list
 *
 *  \see  TwoDpfoldList()
 */
typedef struct{
  int k;          /**<  \brief  Distance to first reference */
  int l;          /**<  \brief  Distance to second reference */
  FLT_OR_DBL  q;  /**<  \brief  partition function */
} TwoDpfold_solution;

/**
 *  \brief  Variables compound for 2Dfold partition function folding
 *
 *  \see    get_TwoDpfold_variables(), get_TwoDpfold_variables_from_MFE(),
 *          destroy_TwoDpfold_variables(), TwoDpfoldList()
 */
typedef struct{

  unsigned int    alloc;
  char            *ptype;         /**<  \brief  Precomputed array of pair types */
  char            *sequence;      /**<  \brief  The input sequence  */
  short           *S, *S1;        /**<  \brief  The input sequences in numeric form */
  unsigned int    maxD1;          /**<  \brief  Maximum allowed base pair distance to first reference */
  unsigned int    maxD2;          /**<  \brief  Maximum allowed base pair distance to second reference */

  double          temperature;    /* temperature in last call to scale_pf_params */
  double          init_temp;      /* temperature in last call to scale_pf_params */
  FLT_OR_DBL      *scale;
  FLT_OR_DBL      pf_scale;
  pf_paramT       *pf_params;     /* holds all [unscaled] pf parameters */

  int             *my_iindx;      /**<  \brief  Index for moving in quadratic distancy dimensions */
  int             *jindx;         /**<  \brief  Index for moving in the triangular matrix qm1 */

  short           *reference_pt1;
  short           *reference_pt2;

  unsigned int    *referenceBPs1; /**<  \brief  Matrix containing number of basepairs of reference structure1 in interval [i,j] */
  unsigned int    *referenceBPs2; /**<  \brief  Matrix containing number of basepairs of reference structure2 in interval [i,j] */
  unsigned int    *bpdist;        /**<  \brief  Matrix containing base pair distance of reference structure 1 and 2 on interval [i,j] */

  unsigned int    *mm1;           /**<  \brief  Maximum matching matrix, reference struct 1 disallowed */
  unsigned int    *mm2;           /**<  \brief  Maximum matching matrix, reference struct 2 disallowed */

  int             circ;
  int             dangles;
  unsigned int    seq_length;

  FLT_OR_DBL      ***Q;
  FLT_OR_DBL      ***Q_B;
  FLT_OR_DBL      ***Q_M;
  FLT_OR_DBL      ***Q_M1;
  FLT_OR_DBL      ***Q_M2;

  FLT_OR_DBL      **Q_c;
  FLT_OR_DBL      **Q_cH;
  FLT_OR_DBL      **Q_cI;
  FLT_OR_DBL      **Q_cM;

  int             **l_min_values;
  int             **l_max_values;
  int             *k_min_values;
  int             *k_max_values;

  int             **l_min_values_b;
  int             **l_max_values_b;
  int             *k_min_values_b;
  int             *k_max_values_b;

  int             **l_min_values_m;
  int             **l_max_values_m;
  int             *k_min_values_m;
  int             *k_max_values_m;

  int             **l_min_values_m1;
  int             **l_max_values_m1;
  int             *k_min_values_m1;
  int             *k_max_values_m1;

  int             **l_min_values_m2;
  int             **l_max_values_m2;
  int             *k_min_values_m2;
  int             *k_max_values_m2;

  int             *l_min_values_qc;
  int             *l_max_values_qc;
  int             k_min_values_qc;
  int             k_max_values_qc;

  int             *l_min_values_qcH;
  int             *l_max_values_qcH;
  int             k_min_values_qcH;
  int             k_max_values_qcH;

  int             *l_min_values_qcI;
  int             *l_max_values_qcI;
  int             k_min_values_qcI;
  int             k_max_values_qcI;

  int             *l_min_values_qcM;
  int             *l_max_values_qcM;
  int             k_min_values_qcM;
  int             k_max_values_qcM;

  /* auxilary arrays for remaining set of coarse graining (k,l) > (k_max, l_max) */
  FLT_OR_DBL      *Q_rem;
  FLT_OR_DBL      *Q_B_rem;
  FLT_OR_DBL      *Q_M_rem;
  FLT_OR_DBL      *Q_M1_rem;
  FLT_OR_DBL      *Q_M2_rem;

  FLT_OR_DBL      Q_c_rem;
  FLT_OR_DBL      Q_cH_rem;
  FLT_OR_DBL      Q_cI_rem;
  FLT_OR_DBL      Q_cM_rem;

} TwoDpfold_vars;

/**
 * \brief Get a datastructure containing all necessary attributes and global folding switches
 *
 * This function prepares all necessary attributes and matrices etc which are needed for a call
 * of vrna_TwoDpfold() .
 * A snapshot of all current global model switches (dangles, temperature and so on) is done and
 * stored in the returned datastructure. Additionally, all matrices that will hold the partition
 * function values are prepared.
 *
 * \param seq         the RNA sequence in uppercase format with letters from the alphabet {AUCG}
 * \param structure1  the first reference structure in dot-bracket notation
 * \param structure2  the second reference structure in dot-bracket notation
 * \param circ        a switch indicating if the sequence is linear (0) or circular (1)
 * \returns           the datastructure containing all necessary partition function attributes
 */
TwoDpfold_vars  *
vrna_TwoDpfold_get_vars(const char *seq,
                        const char *structure1,
                        char *structure2,
                        int circ);

/**
 * \brief Get a datastructure containing all necessary attributes and global folding switches
 *
 * This function prepares all necessary attributes and matrices etc which are needed for a call
 * of vrna_TwoDpfold() .
 * A snapshot of all current global model switches (dangles, temperature and so on) is done and
 * stored in the returned datastructure. Additionally, all matrices that will hold the partition
 * function values are prepared.
 *
 * \deprecated        use vrna_TwoDpfold_get_vars() instead
 *
 * \param seq         the RNA sequence in uppercase format with letters from the alphabet {AUCG}
 * \param structure1  the first reference structure in dot-bracket notation
 * \param structure2  the second reference structure in dot-bracket notation
 * \param circ        a switch indicating if the sequence is linear (0) or circular (1)
 * \returns           the datastructure containing all necessary partition function attributes
 */
DEPRECATED(TwoDpfold_vars  *
get_TwoDpfold_variables(const char *seq,
                        const char *structure1,
                        char *structure2,
                        int circ));

/**
 * \brief Get the datastructure containing all necessary attributes and global folding switches from 
 * a pre-filled mfe-datastructure
 *
 * This function actually does the same as vrna_TwoDpfold_get_vars() but takes its switches and
 * settings from a pre-filled MFE equivalent datastructure
 *
 * \see vrna_TwoDfold_get_vars(), vrna_TwoDpfold_get_vars()
 *
 * \param mfe_vars    the pre-filled mfe datastructure
 * \returns           the datastructure containing all necessary partition function attributes
 */
TwoDpfold_vars  *
vrna_TwoDpfold_get_vars_from_MFE(TwoDfold_vars *mfe_vars);

/**
 * \brief Get the datastructure containing all necessary attributes and global folding switches from 
 * a pre-filled mfe-datastructure
 *
 * This function actually does the same as vrna_TwoDpfold_get_vars() but takes its switches and
 * settings from a pre-filled MFE equivalent datastructure
 *
 * \deprecated        use vrna_TwoDpfold_get_vars_from_MFE() instead
 *
 * \see vrna_TwoDfold_get_vars(), vrna_TwoDpfold_get_vars()
 *
 * \param mfe_vars    the pre-filled mfe datastructure
 * \returns           the datastructure containing all necessary partition function attributes
 */
DEPRECATED(TwoDpfold_vars  *
get_TwoDpfold_variables_from_MFE(TwoDfold_vars *mfe_vars));


/**
 * \brief Free all memory occupied by a TwoDpfold_vars datastructure
 *
 * This function free's all memory occupied by a datastructure obtained from from
 * vrna_TwoDpfold_get_vars() or vrna_TwoDpfold_get_vars_from_MFE()
 *
 * \see vrna_TwoDpfold_get_vars(), vrna_TwoDpfold_get_vars_from_MFE()
 *
 * \param vars   the datastructure to be free'd
 */
void
vrna_TwoDpfold_destroy_vars(TwoDpfold_vars *vars);

/**
 * \brief Free all memory occupied by a TwoDpfold_vars datastructure
 *
 * This function free's all memory occupied by a datastructure obtained from from
 * vrna_TwoDpfold_get_vars() or vrna_TwoDpfold_get_vars_from_MFE()
 *
 * \deprecated see vrna_TwoDpfold_destroy_vars() as a substitute
 *
 * \see vrna_TwoDpfold_get_vars(), vrna_TwoDpfold_get_vars_from_MFE()
 *
 * \param vars   the datastructure to be free'd
 */
DEPRECATED(void 
destroy_TwoDpfold_variables(TwoDpfold_vars *vars));

/**
 * \brief Compute the partition function for all distance classes
 *
 * This function computes the partition functions for all distance classes
 * according the two reference structures specified in the datastructure 'vars'.
 * Similar to vrna_TwoDfold() the arguments maxDistance1 and maxDistance2 specify
 * the maximum distance to both reference structures. A value of '-1' in either of
 * them makes the appropriate distance restrictionless, i.e. all basepair distancies
 * to the reference are taken into account during computation.
 * In case there is a restriction, the returned solution contains an entry where
 * the attribute k=l=-1 contains the partition function for all structures exceeding
 * the restriction.
 * A values of #INF in the attribute 'k' of the returned list denotes the end of the list
 *
 * \see vrna_TwoDpfold_get_vars(), destroy_TwoDpfold_variables(), #TwoDpfold_solution
 *
 * \param vars          the datastructure containing all necessary folding attributes and matrices
 * \param maxDistance1  the maximum basepair distance to reference1 (may be -1)
 * \param maxDistance2  the maximum basepair distance to reference2 (may be -1)
 * \returns             a list of partition funtions for the appropriate distance classes
 */
TwoDpfold_solution  *
vrna_TwoDpfold( TwoDpfold_vars *vars,
                int maxDistance1,
                int maxDistance2);

/**
 * \brief Compute the partition function for all distance classes
 *
 * This function computes the partition functions for all distance classes
 * according the two reference structures specified in the datastructure 'vars'.
 * Similar to vrna_TwoDfold() the arguments maxDistance1 and maxDistance2 specify
 * the maximum distance to both reference structures. A value of '-1' in either of
 * them makes the appropriate distance restrictionless, i.e. all basepair distancies
 * to the reference are taken into account during computation.
 * In case there is a restriction, the returned solution contains an entry where
 * the attribute k=l=-1 contains the partition function for all structures exceeding
 * the restriction.
 * A values of #INF in the attribute 'k' of the returned list denotes the end of the list
 *
 * \deprecated use vrna_TwoDpfold() instead
 *
 * \see vrna_TwoDpfold_get_vars(), destroy_TwoDpfold_variables(), #TwoDpfold_solution
 *
 * \param vars          the datastructure containing all necessary folding attributes and matrices
 * \param maxDistance1  the maximum basepair distance to reference1 (may be -1)
 * \param maxDistance2  the maximum basepair distance to reference2 (may be -1)
 * \returns             a list of partition funtions for the appropriate distance classes
 */
DEPRECATED(TwoDpfold_solution  *
TwoDpfoldList(TwoDpfold_vars *vars,
              int maxDistance1,
              int maxDistance2));

/** @} */ /* End of group kl_neighborhood_pf */

/**
 *  \addtogroup kl_neighborhood_stochbt
 *  \brief Contains functions related to stochastic backtracking from a specified distance class
 *  @{
 */

/**
 *  \brief Sample secondary structure representatives from a set of distance classes according to their 
 *  Boltzmann probability
 *
 *  If the argument 'd1' is set to '-1', the structure will be backtracked in the distance class
 *  where all structures exceeding the maximum basepair distance to either of the references reside.
 *
 *  \pre      The argument 'vars' must contain precalculated partition function matrices,
 *            i.e. a call to vrna_TwoDpfold() preceding this function is mandatory!
 *
 *  \see      vrna_TwoDpfold()
 *
 *  \param[in]  vars  the datastructure containing all necessary folding attributes and matrices
 *  \param[in]  d1    the distance to reference1 (may be -1)
 *  \param[in]  d2    the distance to reference2
 *  \returns    A sampled secondary structure in dot-bracket notation
 */
char *
vrna_TwoDpfold_pbacktrack(TwoDpfold_vars *vars,
                          int d1,
                          int d2);

/**
 *  \brief Sample secondary structure representatives from a set of distance classes according to their 
 *  Boltzmann probability
 *
 *  If the argument 'd1' is set to '-1', the structure will be backtracked in the distance class
 *  where all structures exceeding the maximum basepair distance to either of the references reside.
 *
 *  \pre      The argument 'vars' must contain precalculated partition function matrices,
 *            i.e. a call to vrna_TwoDpfold() preceding this function is mandatory!
 *
 *  \deprecated use vrna_TwoDpfold_pbacktrack() instead
 *
 *  \see      vrna_TwoDpfold()
 *
 *  \param[in]  vars  the datastructure containing all necessary folding attributes and matrices
 *  \param[in]  d1    the distance to reference1 (may be -1)
 *  \param[in]  d2    the distance to reference2
 *  \returns    A sampled secondary structure in dot-bracket notation
 */
DEPRECATED(char *
TwoDpfold_pbacktrack( TwoDpfold_vars *vars,
                      int d1,
                      int d2));

/**
 * \brief Sample secondary structure representatives with a specified length from a set of distance classes according to their 
 *  Boltzmann probability
 *
 * This function does essentially the same as vrna_TwoDpfold_pbacktrack() with the only difference that partial structures,
 * i.e. structures beginning from the 5' end with a specified length of the sequence, are backtracked
 *
 * \note      This function does not work (since it makes no sense) for circular RNA sequences!
 * \pre       The argument 'vars' must contain precalculated partition function matrices,
 *            i.e. a call to vrna_TwoDpfold() preceding this function is mandatory!
 *
 * \see       TwoDpfold_pbacktrack(), vrna_TwoDpfold()
 *
 *  \param[in]  vars    the datastructure containing all necessary folding attributes and matrices
 *  \param[in]  d1      the distance to reference1 (may be -1)
 *  \param[in]  d2      the distance to reference2
 *  \param[in]  length  the length of the structure beginning from the 5' end
 *  \returns    A sampled secondary structure in dot-bracket notation
 */
char *
vrna_TwoDpfold_pbacktrack5( TwoDpfold_vars *vars,
                            int d1,
                            int d2,
                            unsigned int length);

/**
 * \brief Sample secondary structure representatives with a specified length from a set of distance classes according to their 
 *  Boltzmann probability
 *
 * This function does essentially the same as vrna_TwoDpfold_pbacktrack() with the only difference that partial structures,
 * i.e. structures beginning from the 5' end with a specified length of the sequence, are backtracked
 *
 * \note      This function does not work (since it makes no sense) for circular RNA sequences!
 * \pre       The argument 'vars' must contain precalculated partition function matrices,
 *            i.e. a call to vrna_TwoDpfold() preceding this function is mandatory!
 *
 * \deprecated use vrna_TwoDpfold_pbacktrack5() instead
 *
 * \see       TwoDpfold_pbacktrack(), vrna_TwoDpfold()
 *
 *  \param[in]  vars    the datastructure containing all necessary folding attributes and matrices
 *  \param[in]  d1      the distance to reference1 (may be -1)
 *  \param[in]  d2      the distance to reference2
 *  \param[in]  length  the length of the structure beginning from the 5' end
 *  \returns    A sampled secondary structure in dot-bracket notation
 */
DEPRECATED(char *
TwoDpfold_pbacktrack5(TwoDpfold_vars *vars,
                      int d1,
                      int d2,
                      unsigned int length));

/** @} */ /* End of group kl_neighborhood_stochbt */

/**
 * \brief
 *
 *
 */
DEPRECATED(FLT_OR_DBL          **TwoDpfold(TwoDpfold_vars *our_variables,
                                int maxDistance1,
                                int maxDistance2));

/**
 * \brief
 *
 *
 */
DEPRECATED(FLT_OR_DBL          **TwoDpfold_circ(
                                TwoDpfold_vars *our_variables,
                                int maxDistance1,
                                int maxDistance2));

#endif
