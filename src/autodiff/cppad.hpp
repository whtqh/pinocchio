//
// Copyright (c) 2018-2019 CNRS INRIA
//

#ifndef __pinocchio_autodiff_ccpad_hpp__
#define __pinocchio_autodiff_ccpad_hpp__

#include "pinocchio/math/fwd.hpp"
#include "pinocchio/spatial/explog.hpp"

// Do not include this file directly.
// Copy and use directly the intructions from <cppad/example/cppad_eigen.hpp>
// to avoid redifinition of EIGEN_MATRIXBASE_PLUGIN for Eigen 3.3.0 and later
//#include <cppad/example/cppad_eigen.hpp>

#ifdef PINOCCHIO_CPPAD_REQUIRES_MATRIX_BASE_PLUGIN
  #define EIGEN_MATRIXBASE_PLUGIN <cppad/example/eigen_plugin.hpp>
#endif

#include <cppad/cppad.hpp>
#include <Eigen/Dense>

namespace boost
{
  namespace math
  {
    namespace constants
    {
      namespace detail
      {
        template<typename Scalar>
        struct constant_pi< CppAD::AD<Scalar> > : constant_pi<Scalar> {
          template <int N>
          static inline CppAD::AD<Scalar> get(const mpl::int_<N>& n)
          {
            return CppAD::AD<Scalar>(constant_pi<Scalar>::get(n));
          }

        };
      }
    }
  }
}

namespace Eigen
{
  namespace internal
  {
    // Specialization of Eigen::internal::cast_impl for CppAD input types
    template<typename Scalar>
    struct cast_impl<CppAD::AD<Scalar>,Scalar>
    {
#if EIGEN_VERSION_AT_LEAST(3,2,90)
      EIGEN_DEVICE_FUNC
#endif
      static inline Scalar run(const CppAD::AD<Scalar> & x)
      {
        return CppAD::Value(x);
      }
    };
  }
} //namespace Eigen

// Source from #include <cppad/example/cppad_eigen.hpp>
namespace Eigen
{
  template <class Base> struct NumTraits< CppAD::AD<Base> >
  {  // type that corresponds to the real part of an AD<Base> value
    typedef CppAD::AD<Base>   Real;
    // type for AD<Base> operations that result in non-integer values
    typedef CppAD::AD<Base>   NonInteger;
    //  type to use for numeric literals such as "2" or "0.5".
    typedef CppAD::AD<Base>   Literal;
    // type for nested value inside an AD<Base> expression tree
    typedef CppAD::AD<Base>   Nested;
    
    enum {
      // does not support complex Base types
      IsComplex             = 0 ,
      // does not support integer Base types
      IsInteger             = 0 ,
      // only support signed Base types
      IsSigned              = 1 ,
      // must initialize an AD<Base> object
      RequireInitialization = 1 ,
      // computational cost of the corresponding operations
      ReadCost              = 1 ,
      AddCost               = 2 ,
      MulCost               = 2
    };
    
    // machine epsilon with type of real part of x
    // (use assumption that Base is not complex)
    static CppAD::AD<Base> epsilon(void)
    {  return CppAD::numeric_limits< CppAD::AD<Base> >::epsilon(); }
    
    // relaxed version of machine epsilon for comparison of different
    // operations that should result in the same value
    static CppAD::AD<Base> dummy_precision(void)
    {  return 100. *
      CppAD::numeric_limits< CppAD::AD<Base> >::epsilon();
    }
    
    // minimum normalized positive value
    static CppAD::AD<Base> lowest(void)
    {  return CppAD::numeric_limits< CppAD::AD<Base> >::min(); }
    
    // maximum finite value
    static CppAD::AD<Base> highest(void)
    {  return CppAD::numeric_limits< CppAD::AD<Base> >::max(); }
    
    // number of decimal digits that can be represented without change.
    static int digits10(void)
    {  return CppAD::numeric_limits< CppAD::AD<Base> >::digits10; }
  };
} // namespace Eigen

// Source from #include <cppad/example/cppad_eigen.hpp>
namespace CppAD
{
  // functions that return references
  template <class Base> const AD<Base>& conj(const AD<Base>& x)
  {  return x; }
  template <class Base> const AD<Base>& real(const AD<Base>& x)
  {  return x; }
  
  // functions that return values (note abs is defined by cppad.hpp)
  template <class Base> AD<Base> imag(const AD<Base>& /*x*/)
  {  return CppAD::AD<Base>(0.); }
  template <class Base> AD<Base> abs2(const AD<Base>& x)
  {  return x * x; }
} // namespace CppAD

namespace pinocchio
{
  template<typename Scalar>
  struct TaylorSeriesExpansion< CppAD::AD<Scalar> > : TaylorSeriesExpansion<Scalar>
  {
    typedef TaylorSeriesExpansion<Scalar> Base;
    //using Base::precision;

    template<int degree>
    static CppAD::AD<Scalar> precision()
    {
      return CppAD::AD<Scalar>(Base::template precision<degree>());
    }

  };

  /// \brief Implementation of log6 for overloaded CppAD type.
  template<typename Scalar, int Options>
  struct log6Algo<CppAD::AD<Scalar>, Options, false>
  {
    static void run(const SE3Tpl<CppAD::AD<Scalar>,Options> & M,
                    MotionTpl<CppAD::AD<Scalar>,Options>& mout)
    {
      typedef CppAD::AD<Scalar> ADScalar;
      typedef SE3Tpl<ADScalar,Options> SE3;
      typedef MotionTpl<ADScalar,Options> Motion;
      typedef typename SE3::Vector3 Vector3;
      
      typename SE3::ConstAngularRef R = M.rotation();
      typename SE3::ConstLinearRef p = M.translation();
      
      ADScalar t;
      Vector3 w(log3(R,t)); // t in [0,π]
      const ADScalar t2 = t*t;
      ADScalar alpha, beta;
      ADScalar st,ct; SINCOS(t,&st,&ct);
      alpha = CppAD::CondExpLt<Scalar>(t, TaylorSeriesExpansion<ADScalar>::template precision<3>(),
                               /*true*/ ADScalar(1) - t2/ADScalar(12) - t2*t2/ADScalar(720),
                               /*false*/ t*st/(ADScalar(2)*(ADScalar(1)-ct)));
      beta = CppAD::CondExpLt<Scalar>(t, TaylorSeriesExpansion<ADScalar>::template precision<3>(),
                              /*true*/ADScalar(1)/ADScalar(12) + t2/ADScalar(720),
                              /*false*/ADScalar(1)/t2 - st/(ADScalar(2)*t*(ADScalar(1)-ct)));
      mout.linear() = alpha * p - ADScalar(0.5) * w.cross(p) + beta * w.dot(p) * w;
      mout.angular() = w;
    }
  };

  //\brief Generic evaluation of log3 function
  template<typename ADMatrix3Like, typename _Scalar>
  struct log3Algo<ADMatrix3Like, CppAD::AD<_Scalar>, false>
  {
    static void run(const Eigen::MatrixBase<ADMatrix3Like> & R,
                    typename ADMatrix3Like::Scalar & theta,
                    Eigen::Matrix<typename ADMatrix3Like::Scalar,3,1,
                    PINOCCHIO_EIGEN_PLAIN_TYPE(ADMatrix3Like)::Options>& res)
    {
      PINOCCHIO_ASSERT_MATRIX_SPECIFIC_SIZE(ADMatrix3Like, R, 3, 3);

      typedef typename ADMatrix3Like::Scalar ADScalar;
      typedef typename ADMatrix3Like::Scalar::value_type Scalar;
      
      typedef Eigen::Matrix<ADScalar,3,1,PINOCCHIO_EIGEN_PLAIN_TYPE(ADMatrix3Like)::Options> ADVector3;
    
      static const ADScalar PI_value = PI<ADScalar>();
      const ADScalar tr = R.trace();
      theta = CppAD::CondExpLt<Scalar>(tr, ADScalar(-1),
                                       PI_value,
                                       CppAD::CondExpGt<Scalar>(tr, Scalar(3),
                                                                ADScalar(0),
                                                                math::acos((tr - ADScalar(1))/ADScalar(2))));
                                       
      assert(theta == theta && "theta contains some NaN"); // theta != NaN

      const ADScalar cphi = cos(theta - PI_value);
      const ADScalar beta  = theta*theta / ( ADScalar(1) + cphi );
      ADVector3 tmp((R.diagonal().array() + cphi) * beta);
      const ADScalar t = CppAD::CondExpGt<Scalar>(theta, TaylorSeriesExpansion<ADScalar>::template precision<3>()
                                                  ,theta / sin(theta)
                                                  ,ADScalar(1)) / ADScalar(2);
      
      res(0) = CppAD::CondExpGt<Scalar>(theta, PI_value-Scalar(1e-2),
                                        CppAD::CondExpGt<Scalar>(R (2, 1), R (1, 2), ADScalar(1), ADScalar(-1)) *
                                        CppAD::CondExpGt<Scalar>(tmp[0], ADScalar(0), sqrt(tmp[0]), ADScalar(0)),
                                        t * (R (2, 1) - R (1, 2)));
      
      res(1) = CppAD::CondExpGt<Scalar>(theta, PI_value-Scalar(1e-2),
                                        CppAD::CondExpGt<Scalar>(R (0, 2), R (2, 0), ADScalar(1), ADScalar(-1)) *
                                        CppAD::CondExpGt<Scalar>(tmp[1], ADScalar(0), sqrt(tmp[1]), ADScalar(0)),
                                        t * (R (0, 2) - R (2, 0)));
      
      res(2) = CppAD::CondExpGt<Scalar>(theta, PI_value-Scalar(1e-2),
                                        CppAD::CondExpGt<Scalar>(R (1, 0), R (0, 1), ADScalar(1), ADScalar(-1)) *
                                        CppAD::CondExpGt<Scalar>(tmp[2], ADScalar(0), sqrt(tmp[2]), ADScalar(0)),
                                        t * (R (1, 0) - R (0, 1)));
    }
  };


  

  
} // namespace pinocchio

#endif // #ifndef __pinocchio_autodiff_ccpad_hpp__
