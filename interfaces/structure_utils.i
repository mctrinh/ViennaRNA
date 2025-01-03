/*#######################################*/
/* Structure Utitlities section          */
/*#######################################*/
%{
#include <sstream>
%}

/* first, the ignore list */
%ignore pack_structure;
%ignore unpack_structure;
%ignore make_pair_table;
%ignore make_pair_table_pk;
%ignore copy_pair_table;
%ignore alimake_pair_table;
%ignore make_pair_table_snoop;
%ignore make_loop_index_pt;
%ignore bp_distance;
%ignore make_referenceBP_array;
%ignore compute_BPdifferences;
%ignore parenthesis_structure;
%ignore parenthesis_zuker;
%ignore letter_structure;
%ignore bppm_to_structure;
%ignore bppm_symbol;
%ignore plist;
%ignore assign_plist_from_db;
%ignore assign_plist_from_pr;

//%ignore vrna_hx_t;
//%ignore vrna_hx_s;
%ignore vrna_ptable_from_string;
%ignore vrna_db_flatten;
%ignore vrna_db_flatten_to;
%ignore vrna_db_from_WUSS;


/************************************/
/* Relevant data structure wrappers */
/************************************/

/*
 *  Rename 'struct vrna_ep_t' to target language class 'ep'
 *  and create necessary wrapper extensions
 */

%nodefaultctor vrna_ep_t;

%rename (ep) vrna_ep_t;

%newobject vrna_ep_t::__str__;

typedef struct {
  int i;
  int j;
  float p;
  int type;
} vrna_ep_t;

%extend vrna_ep_t {

    vrna_ep_t(unsigned int  i,
              unsigned int  j,
              float         p     = 1.,
              int           type  = VRNA_PLIST_TYPE_BASEPAIR)
    {
      vrna_ep_t *pair;

      pair        = (vrna_ep_t *)vrna_alloc(sizeof(vrna_ep_t));
      pair->i     = (int)i;
      pair->j     = (int)j;
      pair->p     = p;
      pair->type  = type;

      return pair;
    }

#ifdef SWIGPYTHON
    std::string
    __str__()
    {
      std::ostringstream out;
      out << "{ i: " << $self->i;
      out << ", j: " << $self->j;
      out << ", p: " << $self->p;
      out << ", t: " << $self->type;
      out << " }";

      return std::string(out.str());
    }

%pythoncode %{
def __repr__(self):
    # reformat string representation (self.__str__()) to something
    # that looks like a constructor argument list
    strthis = self.__str__().replace(": ", "=").replace("{ ", "").replace(" }", "")
    return  "%s.%s(%s)" % (self.__class__.__module__, self.__class__.__name__, strthis) 
%}
#endif
}


/************************************/
/*  Dot-Bracket wrappers            */
/************************************/
%rename (db_from_ptable) my_db_from_ptable;
%rename (db_pack) vrna_db_pack;
%rename (db_unpack) vrna_db_unpack;
%rename (pack_structure) my_pack_structure;
%rename (unpack_structure) my_unpack_structure;
%rename (db_to_element_string) vrna_db_to_element_string;

%newobject my_unpack_structure;
%newobject my_pack_structure;
%newobject my_db_from_ptable;
%newobject vrna_db_unpack;
%newobject vrna_db_pack;
%newobject vrna_db_to_element_string;

%{
#include <vector>
  char *
  my_pack_structure(const char *s)
  {
    return vrna_db_pack(s);
  }

  char *
  my_unpack_structure(const char *packed)
  {
    return vrna_db_unpack(packed);
  }

  char *
  my_db_from_ptable(std::vector<int> pt)
  {
    std::vector<short> vc;
    transform(pt.begin(), pt.end(), back_inserter(vc), convert_vecint2vecshort);
    return vrna_db_from_ptable((short*)&vc[0]);
  }

  char *
  my_db_from_ptable(var_array<short> const &pt)
  {
    return vrna_db_from_ptable(pt.data);
  }

  void
  db_flatten(char         *structure,
             unsigned int options = VRNA_BRACKETS_DEFAULT)
  {
    vrna_db_flatten(structure, options);
  }

  void
  db_flatten(char         *structure,
             std::string  target,
             unsigned int options = VRNA_BRACKETS_DEFAULT)
  {
    if (target.size() == 2)
      vrna_db_flatten_to(structure, target.c_str(), options);
    else
      vrna_log_warning("db_flatten(): target pair must be string of exactly 2 characters!");
  }

  std::string
  db_from_WUSS(std::string wuss)
  {
    char *c_str = vrna_db_from_WUSS(wuss.c_str());
    std::string db = c_str;
    free(c_str);
    return db;
  }

  std::string
  abstract_shapes(std::string   structure,
                  unsigned int  level = 5)
  {
    if (structure.size() == 0)
      return structure;

    char *c_str = vrna_abstract_shapes(structure.c_str(), level);
    std::string SHAPE = c_str;
    free(c_str);
    return SHAPE;
  }

  std::string
  abstract_shapes(std::vector<int> pt,
                  unsigned int     level = 5)
  {
    if (pt.size() == 0)
      return "";

    std::vector<short> vc;
    transform(pt.begin(), pt.end(), back_inserter(vc), convert_vecint2vecshort);

    char *c_str = vrna_abstract_shapes_pt((short*)&vc[0], level);

    std::string SHAPE = c_str;
    free(c_str);
    return SHAPE;
  }

  std::string
  abstract_shapes(var_array<short> const &pt,
                  unsigned int     level = 5)
  {
    char *c_str = vrna_abstract_shapes_pt(pt.data, level);

    std::string SHAPE = c_str;
    free(c_str);
    return SHAPE;
  }
%}


#ifdef SWIGPYTHON
%feature("autodoc") my_db_from_ptable;
%feature("kwargs") my_db_from_ptable;
%feature("autodoc") my_pack_structure;
%feature("kwargs") my_pack_structure;
%feature("autodoc") my_unpack_structure;
%feature("kwargs") my_unpack_structure;
%feature("autodoc") abstract_shapes;
%feature("kwargs") abstract_shapes;
#endif

char        *my_pack_structure(const char *s);
char        *my_unpack_structure(const char *packed);
char        *my_db_from_ptable(std::vector<int> pt);
char        *my_db_from_ptable(var_array<short> const &pt);
void        db_flatten(char *structure, unsigned int options = VRNA_BRACKETS_DEFAULT);
void        db_flatten(char *structure, std::string target, unsigned int options = VRNA_BRACKETS_DEFAULT);
std::string db_from_WUSS(std::string wuss);
std::string abstract_shapes(std::string structure, unsigned int  level = 5);
std::string abstract_shapes(std::vector<int> pt, unsigned int  level = 5);
std::string abstract_shapes(var_array<short> const &pt, unsigned int  level = 5);


/************************************/
/*  Pairtable wrappers              */
/************************************/
%rename (ptable) my_ptable;
%rename (ptable_from_string) my_ptable_from_string;
%rename (ptable_pk) my_ptable_pk;
%rename (pt_pk_remove)  my_pt_pk_remove;

%{
#include <vector>

#ifdef SWIGPYTHON
  var_array<short int> *
  my_ptable(std::string   str,
            unsigned int  options = VRNA_BRACKETS_RND)
  {
    short int       *pt;
    int             i;
    var_array<short int>  *v_pt;

    pt = vrna_ptable_from_string(str.c_str(), options);
    v_pt = var_array_new(str.size(),
                         pt,
                         VAR_ARRAY_LINEAR | VAR_ARRAY_ONE_BASED | VAR_ARRAY_OWNED);

    return v_pt;
  }

#else
  std::vector<int>
  my_ptable(std::string   str,
            unsigned int  options = VRNA_BRACKETS_RND)
  {
    short int         *pt;
    int               i;
    std::vector<int>  v_pt;

    pt = vrna_ptable_from_string(str.c_str(), options);

    for(i = 0; i <= pt[0]; i++)
      v_pt.push_back(pt[i]);

    free(pt);
    return v_pt;
  }
#endif

  std::vector<int>
  my_ptable_pk(std::string str)
  {
    short int* pt_pk = vrna_pt_pk_get(str.c_str());
    std::vector<int> v_pt;
    int i;

    for(i=0; i <= pt_pk[0]; i++){
      v_pt.push_back(pt_pk[i]);
    }
    free(pt_pk);
    return v_pt; 
  }

  std::vector<int>
  my_pt_pk_remove(std::vector<int>  pt,
                  unsigned int      options = 0)
  {
    short               *ptable;
    int                 i;
    std::vector<short>  vs;
    std::vector<int>    v_pt;

    /* sanity check and fix */
    if (pt[0] != pt.size() - 1)
      pt[0] = pt.size() - 1;

    transform(pt.begin(), pt.end(), back_inserter(vs), convert_vecint2vecshort);

    ptable = vrna_pt_pk_remove((const short*)&vs[0], options);

    for (i = 0; i <= ptable[0]; i++)
      v_pt.push_back(ptable[i]);

    free(ptable);

    return v_pt;
  }

  var_array<short> *
  my_pt_pk_remove(var_array<short> const &pt,
                  unsigned int      options = 0)
  {
    short *ptable = vrna_pt_pk_remove(pt.data, options);

    return var_array_new(ptable[0],
                         ptable,
                         VAR_ARRAY_LINEAR | VAR_ARRAY_ONE_BASED | VAR_ARRAY_OWNED);
  }

%}

#ifdef SWIGPYTHON
%feature("autodoc") my_ptable;
%feature("kwargs") my_ptable;
%feature("autodoc") my_ptable_pk;
%feature("kwargs") my_ptable_pk;
%feature("autodoc") my_pt_pk_remove;
%feature("kwargs") my_pt_pk_remove;
#endif

#ifdef SWIGPYTHON
var_array<short int> * my_ptable(std::string   str, unsigned int  options = VRNA_BRACKETS_RND);
#else
std::vector<int> my_ptable(std::string str, unsigned int options = VRNA_BRACKETS_RND);
#endif

std::vector<int> my_ptable_pk(std::string str);
std::vector<int> my_pt_pk_remove(std::vector<int> pt, unsigned int options = 0);
var_array<short> *my_pt_pk_remove(var_array<short> const &pt, unsigned int options = 0);

%ignore vrna_pt_pk_remove;

/************************************/
/*  Pairlist wrappers (vrna_ep_t)   */
/************************************/
%rename (plist) my_plist;

%{
#include <vector>
  std::vector<vrna_ep_t>
  my_plist(std::string  structure,
           float        pr = 0.95*0.95)
  {
    std::vector<vrna_ep_t > ep_v;
    vrna_ep_t               *ptr, *plist;

    plist = vrna_plist(structure.c_str(), pr);

    for (ptr = plist; ptr->i && ptr->j; ptr++) {
      vrna_ep_t pl;
      pl.i = ptr->i;
      pl.j = ptr->j;
      pl.p = ptr->p;
      pl.type = ptr->type;
      ep_v.push_back(pl);
    }

    free(plist);

    return ep_v;
  }

  std::string
  db_from_plist(std::vector<vrna_ep_t> pairs,
                unsigned int           length)
  {
    vrna_ep_t last_elem;
    last_elem.i     = last_elem.j = 0;
    last_elem.p     = 0;
    last_elem.type  = 0;

    pairs.push_back(last_elem);

    char *str = vrna_db_from_plist(&pairs[0], length);
    std::string ret(str);
    free(str);

    /* remove end-of-list marker */
    pairs.pop_back();

    return ret;
  }

  std::string
  db_pk_remove(std::string  structure,
               unsigned int options = VRNA_BRACKETS_ANY)
  {
    char *db = vrna_db_pk_remove(structure.c_str(), options);
    std::string ret(db);
    free(db);

    return ret;
  }
%}

#ifdef SWIGPYTHON
%feature("autodoc") my_plist;
%feature("kwargs") my_plist;
%feature("autodoc") db_from_plist;
%feature("kwargs") db_from_plist;
%feature("autodoc") db_pk_remove;
%feature("kwargs") db_pk_remove;
#endif

std::vector<vrna_ep_t> my_plist(std::string structure, float pr);
std::string db_from_plist(std::vector<vrna_ep_t> elem_probs, unsigned int length);
std::string db_pk_remove(std::string structure, unsigned int options = VRNA_BRACKETS_ANY);


%extend vrna_fold_compound_t {
#include <vector>

#ifdef SWIGPYTHON
%feature("autodoc") plist_from_probs;
%feature("kwargs") plist_from_probs;
#endif

  std::vector<vrna_ep_t>
  plist_from_probs(double cutoff)
  {
    std::vector<vrna_ep_t > ep_v;
    vrna_ep_t               *ptr, *plist;

    plist = vrna_plist_from_probs($self, cutoff);

    for (ptr = plist; ptr->i && ptr->j; ptr++) {
      vrna_ep_t pl;
      pl.i = ptr->i;
      pl.j = ptr->j;
      pl.p = ptr->p;
      pl.type = ptr->type;
      ep_v.push_back(pl);
    }

    free(plist);

    return ep_v;
  }

  std::string
  db_from_probs(void)
  {
    if (($self->exp_matrices) &&
        ($self->exp_matrices->probs)) {
      char *propensities = vrna_db_from_probs($self->exp_matrices->probs,
                                              $self->length);

      std::string prop_string(propensities);

      free(propensities);

      return prop_string;
    }

    return std::string("");
  }
}

/************************************/
/*  Tree representation wrappers    */
/************************************/

%{
  std::string
  db_to_tree_string(std::string   structure,
                    unsigned int  type)
  {
    char *c_str = vrna_db_to_tree_string(structure.c_str(), type);
    std::string tree = c_str;
    free(c_str);
    return tree;
  }

  std::string
  tree_string_unweight(std::string structure)
  {
    char *c_str = vrna_tree_string_unweight(structure.c_str());
    std::string tree = c_str;
    free(c_str);
    return tree;
  }

  std::string
  tree_string_to_db(std::string structure)
  {
    char *c_str = vrna_tree_string_to_db(structure.c_str());
    std::string db = c_str;
    free(c_str);
    return db;
  }

%}

#ifdef SWIGPYTHON
%feature("autodoc") db_to_tree_string;
%feature("kwargs") db_to_tree_string;
#endif

std::string db_to_tree_string(std::string structure, unsigned int type);
std::string tree_string_unweight(std::string structure);
std::string tree_string_to_db(std::string structure);

%inline %{
  short *
  make_loop_index(const char *structure)
  {
    /* number each position by which loop it belongs to (positions start at 0) */
    int i,hx,l,nl;
    int length;
    short *stack;
    short *loop;
    length = strlen(structure);
    stack = (short *) vrna_alloc(sizeof(short)*(length+1));
    loop = (short *) vrna_alloc(sizeof(short)*(length+2));
    hx=l=nl=0;
    for (i=0; i<length; i++) {
      if (structure[i] == '(') {
        nl++; l=nl;
        stack[hx++]=i;
      }
      loop[i]=l;
      if (structure[i] ==')') {
        --hx;
        if (hx>0)
          l = loop[stack[hx-1]];  /* index of enclosing loop   */
        else l=0;                 /* external loop has index 0 */
        if (hx<0) {
          fprintf(stderr, "%s\n", structure);
          nrerror("unbalanced brackets in make_loop_index");
        }
      }
    }
    free(stack);
    return loop;
  }
%}

%rename (loopidx_from_ptable) my_loopidx_from_ptable;

%{
  std::vector<int>
  my_loopidx_from_ptable(std::vector<int> pt)
  {
    int                 i, *idx;
    std::vector<short>  vc;
    std::vector<int>    v_idx;

    transform(pt.begin(), pt.end(), back_inserter(vc), convert_vecint2vecshort);

    idx = vrna_loopidx_from_ptable((short *)&vc[0]);

    v_idx.assign(idx, idx + pt.size());

    free(idx);

    return v_idx;
  }

  var_array<int> *
  my_loopidx_from_ptable(var_array<short> const &pt)
  {
    int *idx = vrna_loopidx_from_ptable(pt.data);
    return var_array_new(pt.data[0], idx, VAR_ARRAY_LINEAR | VAR_ARRAY_ONE_BASED | VAR_ARRAY_OWNED);
  }
%}

#ifdef SWIGPYTHON
%feature("autodoc") my_loopidx_from_ptable;
%feature("kwargs") my_loopidx_from_ptable;
#endif

std::vector<int> my_loopidx_from_ptable(std::vector<int> pt);
var_array<int> *my_loopidx_from_ptable(var_array<short> const &pt);



%nodefaultctor vrna_hx_t;

%rename (hx) vrna_hx_t;

%newobject vrna_hx_t::__str__;

typedef struct {
  unsigned int  start;
  unsigned int  end;
  unsigned int  length;
  unsigned int  up5;
  unsigned int  up3;
} vrna_hx_t;


%extend vrna_hx_t {

    vrna_hx_t(unsigned int  start,
              unsigned int  end,
              unsigned int  length,
              unsigned int  up5 = 0,
              unsigned int  up3 = 0)
    {
      vrna_hx_t *helix;

      helix         = (vrna_hx_t *)vrna_alloc(sizeof(vrna_hx_t));
      helix->start  = start;
      helix->end    = end;
      helix->length = length;
      helix->up5    = up5;
      helix->up3    = up3;

      return helix;
    }

#ifdef SWIGPYTHON
    std::string
    __str__()
    {
      std::ostringstream out;
      out << "{ start: " << $self->start;
      out << ", end: " << $self->end;
      out << ", length: " << $self->length;
      out << ", up5: " << $self->up5;
      out << ", up3: " << $self->up3;
      out << " }";

      return std::string(out.str());
    }

%pythoncode %{
def __repr__(self):
    # reformat string representation (self.__str__()) to something
    # that looks like a constructor argument list
    strthis = self.__str__().replace(": ", "=").replace("{ ", "").replace(" }", "")
    return  "%s.%s(%s)" % (self.__class__.__module__, self.__class__.__name__, strthis) 
%}
#endif
}

%rename (hx_from_ptable) my_hx_from_ptable;
%rename (hx_merge) my_hx_merge;

%{
#include <vector>
  std::vector<vrna_hx_t>
  my_hx_from_ptable(std::vector<int> pt)
  {
    std::vector<vrna_hx_t>  hx_v;
    std::vector<short>      v_pt;
    vrna_hx_t               *ptr, *hxlist;

    transform(pt.begin(), pt.end(), back_inserter(v_pt), convert_vecint2vecshort);

    hxlist = vrna_hx_from_ptable((short *)&v_pt[0]);

    for (ptr = hxlist; ptr->start && ptr->end; ptr++) {
      vrna_hx_t hx;
      hx.start  = ptr->start;
      hx.end    = ptr->end;
      hx.length = ptr->length;
      hx.up5    = ptr->up5;
      hx.up3    = ptr->up3;
      hx_v.push_back(hx);
    }

    free(hxlist);

    return hx_v;
  }

  std::vector<vrna_hx_t>
  my_hx_from_ptable(var_array<short> const &pt)
  {
    std::vector<vrna_hx_t>  hx_v;
    vrna_hx_t               *ptr, *hxlist;

    hxlist = vrna_hx_from_ptable(pt.data);

    for (ptr = hxlist; ptr->start && ptr->end; ptr++) {
      vrna_hx_t hx;
      hx.start  = ptr->start;
      hx.end    = ptr->end;
      hx.length = ptr->length;
      hx.up5    = ptr->up5;
      hx.up3    = ptr->up3;
      hx_v.push_back(hx);
    }

    free(hxlist);

    return hx_v;
  }

  std::vector<vrna_hx_t>
  my_hx_merge(std::vector<vrna_hx_t>  list,
              int                     maxdist = 0)
  {
    std::vector<vrna_hx_t>  hx_merged_v;
    vrna_hx_t               *ptr, *hx_merged;

    vrna_hx_t hx;
    hx.start = hx.end = hx.length = hx.up5 = hx.up3 = 0;
    list.push_back(hx);

    hx_merged = vrna_hx_merge(&(list[0]), maxdist);

    list.pop_back();

    for (ptr = hx_merged; ptr->start && ptr->end; ptr++) {
      vrna_hx_t hx;
      hx.start  = ptr->start;
      hx.end    = ptr->end;
      hx.length = ptr->length;
      hx.up5    = ptr->up5;
      hx.up3    = ptr->up3;
      hx_merged_v.push_back(hx);
    }

    free(hx_merged);

    return hx_merged_v;
  }
%}

#ifdef SWIGPYTHON
%feature("autodoc") my_hx_from_ptable;
%feature("kwargs") my_hx_from_ptable;
#endif

std::vector<vrna_hx_t> my_hx_from_ptable(std::vector<int> pt);
std::vector<vrna_hx_t> my_hx_from_ptable(var_array<short> const &pt);
std::vector<vrna_hx_t> my_hx_merge(std::vector<vrna_hx_t> list, int maxdist = 0);

/************************************/
/*  Distance measures               */
/************************************/
%rename (bp_distance) my_bp_distance;
%rename (dist_mountain) my_dist_mountain;

%{
  int
  my_bp_distance(std::string str1,
                 std::string str2,
                 unsigned int options = VRNA_BRACKETS_RND)
  {
    int dist = 0;
    short int *pt1, *pt2;

    pt1 = vrna_ptable_from_string(str1.c_str(), options);
    pt2 = vrna_ptable_from_string(str2.c_str(), options);

    dist = vrna_bp_distance_pt(pt1, pt2);

    free(pt1);
    free(pt2);

    return dist;
  }

  int
  my_bp_distance(std::vector<int> pt1,
                 std::vector<int> pt2)
  {
    std::vector<short> pt1_v_short;
    std::vector<short> pt2_v_short;

    transform(pt1.begin(), pt1.end(), back_inserter(pt1_v_short), convert_vecint2vecshort);
    transform(pt2.begin(), pt2.end(), back_inserter(pt2_v_short), convert_vecint2vecshort);

    return vrna_bp_distance_pt((short*)&pt1_v_short[0], (short*)&pt2_v_short[0]);
  }

  int
  my_bp_distance(var_array<short> const &pt1,
                 var_array<short> const &pt2)
  {
    return vrna_bp_distance_pt(pt1.data, pt2.data);
  }

  double
  my_dist_mountain( std::string   str1,
                    std::string   str2,
                    unsigned int  p = 1)
  {
    return vrna_dist_mountain(str1.c_str(), str2.c_str(), p);
  }
%}

#ifdef SWIGPYTHON
%feature("autodoc") my_bp_distance;
%feature("kwargs") my_bp_distance;
#endif

int     my_bp_distance(std::string str1, std::string str2, unsigned int options = VRNA_BRACKETS_RND);
int     my_bp_distance(std::vector<int> pt1, std::vector<int> pt2);
int     my_bp_distance(var_array<short> const &pt1, var_array<short> const &pt2);
double  my_dist_mountain(std::string str1, std::string str2, unsigned int p = 1);


/************************************/
/*          Benchmark               */
/************************************/

%nodefaultctor vrna_score_t;

%rename (score) vrna_score_t;

%newobject vrna_score_t::__str__;

typedef struct {
  int TP;
  int TN;
  int FP;
  int FN;
  float TPR;
  float PPV;
  float FPR;
  float FOR;
  float TNR;
  float FDR;
  float FNR;
  float NPV;
  float F1;
  float MCC;
} vrna_score_t;

%extend vrna_score_t {

    vrna_score_t(int TP=0,
                 int TN=0,
                 int FP=0,
                 int FN=0)
    {
      vrna_score_t *score;

      score         = (vrna_score_t *)vrna_alloc(sizeof(vrna_score_t));
      *score = vrna_score_from_confusion_matrix(TP, TN, FP, FN);

      return score;
    }

#ifdef SWIGPYTHON
    std::string
    __str__()
    {
      std::ostringstream out;
      out << "{ TP: " << $self->TP;
      out << ", TN: " << $self->TN;
      out << ", FP: " << $self->FP;
      out << ", FN: " << $self->FN;
      out << ", TPR: " << $self->TPR;
      out << ", PPV: " << $self->PPV;
      out << ", FPR: " << $self->FPR;
      out << ", FOR: " << $self->FOR;
      out << ", TNR: " << $self->TNR;
      out << ", FDR: " << $self->FDR;
      out << ", FNR: " << $self->FNR;
      out << ", NPV: " << $self->NPV;
      out << ", F1: " << $self->F1;
      out << ", MCC: " << $self->MCC;
      out << " }";

      return std::string(out.str());
    }

%pythoncode %{
def __repr__(self):
    # reformat string representation (self.__str__()) to something
    # that looks like a constructor argument list
    strthis = self.__str__().replace(": ", "=").replace("{ ", "").replace(" }", "")
    return  "%s.%s(%s)" % (self.__class__.__module__, self.__class__.__name__, strthis) 
%}
#endif
}


%rename (compare_structure) my_compare_structure;

%{
  vrna_score_t
  my_compare_structure(std::string str1,
                       std::string str2,
                       int fuzzy = 0,
                       unsigned int options = VRNA_BRACKETS_RND)
  {
    return vrna_compare_structure(str1.c_str(), str2.c_str(), fuzzy, options);
  }

  vrna_score_t
  my_compare_structure(std::vector<int> pt1,
                       std::vector<int> pt2,
                       int fuzzy = 0)
  {
    std::vector<short> pt1_v_short;
    std::vector<short> pt2_v_short;

    transform(pt1.begin(), pt1.end(), back_inserter(pt1_v_short), convert_vecint2vecshort);
    transform(pt2.begin(), pt2.end(), back_inserter(pt2_v_short), convert_vecint2vecshort);

    return vrna_compare_structure_pt((short*)&pt1_v_short[0], (short*)&pt2_v_short[0], fuzzy);
  }

  vrna_score_t
  my_compare_structure(var_array<short> const &pt1,
                       var_array<short> const &pt2,
                       int fuzzy = 0)
  {
    return vrna_compare_structure_pt(pt1.data, pt2.data, fuzzy);
  }
%}

#ifdef SWIGPYTHON
%feature("autodoc") my_compare_structure;
%feature("kwargs") my_compare_structure;
#endif

vrna_score_t  my_compare_structure(std::string str1, std::string str2, int fuzzy = 0, unsigned int options = VRNA_BRACKETS_RND);
vrna_score_t  my_compare_structure(std::vector<int> pt1, std::vector<int> pt2, int fuzzy = 0);
vrna_score_t  my_compare_structure(var_array<short> const &pt1, var_array<short> const &pt2, int fuzzy = 0);


%rename(gq_parse)       my_gq_parse;

#ifdef SWIGPYTHON
%feature("autodoc") my_gq_parse;
#endif

%apply  unsigned int *OUTPUT { unsigned int *L };
%apply  std::vector<unsigned int> *OUTPUT { std::vector<unsigned int> *l };

%{
  unsigned int
  my_gq_parse(std::string               structure,
              unsigned int              *L,
              std::vector<unsigned int> *l)
  {
    unsigned int c_L, c_l[3], pos = 0;

    if (structure.size() > 0) {
      *L = 0;
      l->clear();

      pos = vrna_gq_parse(structure.c_str(), &c_L, &(c_l[0]));

      if (pos) {
        *L = c_L;
        l->push_back(c_l[0]);
        l->push_back(c_l[1]);
        l->push_back(c_l[2]);
      }

      return pos;
    }
    
    return pos;
  }

%}

unsigned int
my_gq_parse(std::string               structure,
            unsigned int              *L,
            std::vector<unsigned int> *l);


%clear unsigned int *L;
%clear std::vector<unsigned int> *l;


/************************************/
/*  Wrap constants                  */
/************************************/
%constant int PLIST_TYPE_BASEPAIR = VRNA_PLIST_TYPE_BASEPAIR;
%constant int PLIST_TYPE_GQUAD    = VRNA_PLIST_TYPE_GQUAD;
%constant int PLIST_TYPE_H_MOTIF  = VRNA_PLIST_TYPE_H_MOTIF;
%constant int PLIST_TYPE_I_MOTIF  = VRNA_PLIST_TYPE_I_MOTIF;
%constant int PLIST_TYPE_UD_MOTIF = VRNA_PLIST_TYPE_UD_MOTIF;
%constant int PLIST_TYPE_STACK    = VRNA_PLIST_TYPE_STACK;
%constant int PLIST_TYPE_UNPAIRED = VRNA_PLIST_TYPE_UNPAIRED;

%constant unsigned int STRUCTURE_TREE_HIT            = VRNA_STRUCTURE_TREE_HIT;
%constant unsigned int STRUCTURE_TREE_SHAPIRO_SHORT  = VRNA_STRUCTURE_TREE_SHAPIRO_SHORT;
%constant unsigned int STRUCTURE_TREE_SHAPIRO        = VRNA_STRUCTURE_TREE_SHAPIRO;
%constant unsigned int STRUCTURE_TREE_SHAPIRO_EXT    = VRNA_STRUCTURE_TREE_SHAPIRO_EXT;
%constant unsigned int STRUCTURE_TREE_SHAPIRO_WEIGHT = VRNA_STRUCTURE_TREE_SHAPIRO_WEIGHT;
%constant unsigned int STRUCTURE_TREE_EXPANDED       = VRNA_STRUCTURE_TREE_EXPANDED;

%constant unsigned int BRACKETS_RND     = VRNA_BRACKETS_RND;
%constant unsigned int BRACKETS_ANG     = VRNA_BRACKETS_ANG;
%constant unsigned int BRACKETS_SQR     = VRNA_BRACKETS_SQR;
%constant unsigned int BRACKETS_CLY     = VRNA_BRACKETS_CLY;
%constant unsigned int BRACKETS_ALPHA   = VRNA_BRACKETS_ALPHA;
%constant unsigned int BRACKETS_DEFAULT = VRNA_BRACKETS_DEFAULT;
%constant unsigned int BRACKETS_ANY     = VRNA_BRACKETS_ANY;


%include <ViennaRNA/structures/benchmark.h>
%include <ViennaRNA/structures/dotbracket.h>
%include <ViennaRNA/structures/helix.h>
%include <ViennaRNA/structures/metrics.h>
%include <ViennaRNA/structures/pairtable.h>
%include <ViennaRNA/structures/problist.h>
%include <ViennaRNA/structures/shapes.h>
%include <ViennaRNA/structures/tree.h>
%include <ViennaRNA/structures/utils.h>
