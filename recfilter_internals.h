#ifndef _RECURSIVE_FILTER_INTERNALS_H_
#define _RECURSIVE_FILTER_INTERNALS_H_

#include <vector>
#include <string>
#include <Halide.h>

/** Info about scans in a particular dimension */
struct FilterInfo {
    int                  filter_order;  ///< order of recursive filter in a given dimension
    int                  filter_dim;    ///< dimension id
    int                  num_scans;     ///< number of scans in the dimension that must be tiled
    int                  image_width;   ///< image width in this dimension
    int                  tile_width;    ///< tile width in this dimension
    Halide::Var          var;           ///< variable that represents this dimension
    Halide::RDom         rdom;          ///< RDom update domain of each scan
    std::vector<bool>    scan_causal;   ///< causal or anticausal flag for each scan
    std::vector<int>     scan_id;       ///< scan or update definition id of each scan
};

// ----------------------------------------------------------------------------

enum FunctionTag : int {
    INLINE  = 0x000, ///< function to be removed by inlining
    INTER   = 0x010, ///< filter over tail elements across tiles (single 1D scan)
    INTRA_N = 0x020, ///< filter within tile (multiple scans in multiple dimensions)
    INTRA_1 = 0x040, ///< filter within tile (single scan in one dimension)
    REINDEX = 0x100, ///< function that reindexes a subset of another function to write to global mem
};

enum VariableTag: int {
    INVALID = 0x0000, ///< invalid var
    FULL    = 0x0010, ///< full dimension before tiling
    INNER   = 0x0020, ///< inner dimension after tiling
    OUTER   = 0x0040, ///< outer dimension after tiling
    TAIL    = 0x0080, ///< if dimension is at lower granularity
    SCAN    = 0x0100, ///< if dimension is a scan
    CHANNEL = 0x0200, ///< if dimension represents RGB channels
    __1     = 0x0001, ///< first variable with one of the above tags
    __2     = 0x0002, ///< second variable with one of the above tags
    __3     = 0x0004, ///< third variable with one of the above tags
    __4     = 0x0008, ///< fourth variable with one of the above tags
    SPLIT   = 0x1000, ///< any variable generated by split scheduling operations
};


/** @name Logical operations for scheduling tags */
// {@
VarTag      operator |(const VarTag &a, const VarTag &b);
VarTag      operator &(const VarTag &a, const VarTag &b);
VariableTag operator |(const VariableTag &a, const VariableTag &b);
VariableTag operator &(const VariableTag &a, const VariableTag &b);

bool operator==(const FuncTag &a, const FuncTag &b);
bool operator==(const VarTag  &a, const VarTag &b);
bool operator!=(const FuncTag &a, const FuncTag &b);
bool operator!=(const VarTag  &a, const VarTag &b);
bool operator==(const FuncTag &a, const FunctionTag &b);
bool operator==(const VarTag  &a, const VariableTag &b);
// @}


/** @name Utils to print scheduling tags */
// {@
std::ostream &operator<<(std::ostream &s, const FunctionTag &f);
std::ostream &operator<<(std::ostream &s, const VariableTag &v);
std::ostream &operator<<(std::ostream &s, const FuncTag &f);
std::ostream &operator<<(std::ostream &s, const VarTag &v);
// @}


/** Scheduling tags for Functions */
class FuncTag {
public:
    FuncTag(void)                 : tag(INLINE){}
    FuncTag(const FuncTag     &t) : tag(t.tag) {}
    FuncTag(const FunctionTag &t) : tag(t)     {}
    FuncTag& operator=(const FuncTag     &t) { tag=t.tag; return *this; }
    FuncTag& operator=(const FunctionTag &t) { tag=t;     return *this; }
    int as_integer(void) const { return static_cast<int>(tag); }

private:
    FunctionTag tag;
};

// ----------------------------------------------------------------------------

/** Recursive filter function with scheduling interface */
class RecFilterFunc {
public:
    /** Halide function */
    Halide::Internal::Function func;

    /** Category tag for the function */
    FuncTag func_category;

    /** Category tags for all the pure def vars  */
    std::map<std::string, VarTag> pure_var_category;

    /** Category tags for all the vars in all the update defs */
    std::vector<std::map<std::string,VarTag> >  update_var_category;

    /** List of new vars created by RecFilterSchedule::split() on pure definition */
    std::map<std::string, std::string> pure_var_splits;

    /** List of new vars created by RecFilterSchedule::split() on update definitions */
    std::map<int, std::map<std::string, std::string> >  update_var_splits;

    /** Name of consumer function. This can be set if this function has REINDEX tag set
     * because only REINDEX functions are guaranteed to have a single consumer */
    std::string consumer_func;

    /** Name of producer function. This can be set if this function has REINDEX tag set
     * because only REINDEX functions are guaranteed to have a single producer */
    std::string producer_func;

    /** External consumer function which consumes RecFilter's output;
     * external consumer means a Func outside the RecFilter pipeline that consumes the output
     * of the RecFilter. This is set by RecFilter::compute_at() and is useful for merging the final
     * stage of the recfilter and initial stage of next filter. This can be set if
     * this function has REINDEX tag set; only REINDEX functions are guaranteed to have
     * a single consumer */
    Halide::Func external_consumer_func;

    /** External consumer function's loop level at which the RecFilter's output is consumed;
     * external consumer means a Func outside the RecFilter pipeline that consumes the output
     * of the RecFilter. This is set by RecFilter::compute_at() and is useful for merging the final
     * stage of the recfilter and initial stage of next filter. This can be set if
     * this function has REINDEX tag set; only REINDEX functions are guaranteed to have
     * a single consumer */
    Halide::Var external_consumer_var;

    /** Schedule for pure def of the function as valid Halide code */
    std::vector<std::string> pure_schedule;

    /** Schedule for update defs of the function as valid Halide code */
    std::map<int, std::vector<std::string> > update_schedule;
};

// ----------------------------------------------------------------------------

/** Data members of the recursive filter */
struct RecFilterContents {
    /** Smart pointer */
    mutable Halide::Internal::RefCount ref_count;

    /** Flag to indicate if the filter has been tiled  */
    bool tiled;

    /** Flag to indicate if the filter has been JIT compiled, required before execution */
    bool compiled;

    /** Flag to indicate if the filter has been finalized, required before compilation */
    bool finalized;

    /** Buffer border expression */
    bool clamped_border;

    /** Name of recursive filter as well as function that contains the
     * definition of the filter  */
    std::string name;

    /** Filter output type */
    Halide::Type type;

    /** Info about all the scans in the recursive filter */
    std::vector<FilterInfo> filter_info;

    /** List of functions along with their names and their schedules */
    std::map<std::string, RecFilterFunc> func;

    /** Feed forward coeffs, only one for each scan */
    Halide::Buffer<float> feedfwd_coeff;

    /** Feedback coeffs (num_scans x max_order) order j-th coeff of i-th scan is (i+1,j) */
    Halide::Buffer<float> feedback_coeff;

    /** Compilation and execution target */
    Halide::Target target;
};

#endif // _RECURSIVE_FILTER_INTERNALS_H_