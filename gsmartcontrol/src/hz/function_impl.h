/**************************************************************************
 Copyright:
      (C) 2007 - 2009  Thiago Rosso Adams <thiago.adams 'at' gmail.com>
      (C) 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_zlib.txt file
***************************************************************************/

// TODO: HZIFY

// See function.h for details.


#if FUNC_NUM_ARGS == 0
#define FUNC_USE_COMMA_IF

#define CLASST1_CLASSTN
#define T1_TN              void
#define T1t1_TNtn
#define t1_tn

#else
#define FUNC_USE_COMMA_IF ,

#define CLASST1_CLASSTN    MACRO_MAKE_PARAMS1(FUNC_NUM_ARGS, class T)
#define T1_TN              MACRO_MAKE_PARAMS1(FUNC_NUM_ARGS, T)
#define T1t1_TNtn          MACRO_MAKE_PARAMS2(FUNC_NUM_ARGS, T, t)
#define t1_tn              MACRO_MAKE_PARAMS1(FUNC_NUM_ARGS, t)


#endif

#if FUNC_NUM_ARGS == 1
#define USING_FUNCTION , std::unary_function<T1, Result>
#define USING_FUNCTIONM  : public std::binary_function<T, T1, Result>
#elif FUNC_NUM_ARGS == 2
#define USING_FUNCTION , std::binary_function<T1, T2, Result>
#define USING_FUNCTIONM  //: public std::binary_function<T1, T2, Result>
#else
#define USING_FUNCTION
#define USING_FUNCTIONM
#endif


namespace std {
    namespace tr1 {

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        struct const_mem_fn_t<Result (T::*) (T1_TN) const >
            USING_FUNCTIONM
        {
            typedef Result result_type;
            Result (T::*pMemberFunc) (T1_TN) const;

            const_mem_fn_t(Result (T::*pf) (T1_TN) const) : pMemberFunc(pf)
            {
            }

            Result operator ()(const T *f FUNC_USE_COMMA_IF T1t1_TNtn) const
            {
                return ((f->*pMemberFunc)(t1_tn));
            }

            Result operator ()(const T &f FUNC_USE_COMMA_IF T1t1_TNtn) const
            {
                return ((f.*pMemberFunc)(t1_tn));
            }
        };

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        struct mem_fn_t<Result (T::*) (T1_TN) >
            USING_FUNCTIONM
        {
            typedef Result result_type;
            typedef Result (T::*pointer_type) (T1_TN);
            Result (T::*pMemberFunc) (T1_TN);

            mem_fn_t(Result (T::*pf) (T1_TN)) : pMemberFunc(pf)
            {
            }
            Result operator ()(T *f FUNC_USE_COMMA_IF T1t1_TNtn)
            {
                return ((f->*pMemberFunc)(t1_tn));
            }
            Result operator ()(T &f FUNC_USE_COMMA_IF T1t1_TNtn)
            {
                return ((f.*pMemberFunc)(t1_tn));
            }
        };

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        mem_fn_t<Result (T::*) (T1_TN)> mem_fn(Result (T::*pF) (T1_TN) )
        {
            return mem_fn_t<Result (T::*) (T1_TN)>(pF);
        }


        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        const_mem_fn_t<Result (T::*) (T1_TN) const> mem_fn(Result (T::*pF) (T1_TN) const)
        {
            return const_mem_fn_t<Result (T::*) (T1_TN) const >(pF);
        }

        //bind partially implemented
        //Allows to create function objects that holds "this"
        //X x;
        //f = bind(&X::F, x);

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        struct const_mem_fn_t_binder<Result (T::*) (T1_TN) const>
        {
            const_mem_fn_t<Result (T::*) (T1_TN) const> m_f;
            const T * m_p;

            const_mem_fn_t_binder(Result(T::*PF)(T1_TN) const, const T & r)
                : m_p(&r), m_f(PF)
            {
            }

            const_mem_fn_t_binder(Result(T::*PF)(T1_TN) const, const T *p)
                : m_p(p), m_f(PF)
            {
            }

            Result operator ()(T1t1_TNtn) const
            {
                return m_f(m_p FUNC_USE_COMMA_IF t1_tn);
            }
        };

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        struct mem_fn_t_binder<Result (T::*) (T1_TN) >
        {
            mem_fn_t<Result (T::*) (T1_TN)> m_f;
            T * m_p;

            mem_fn_t_binder(Result(T::*PF)(T1_TN), T & r)
                : m_p(&r), m_f(PF)
            {
            }

            mem_fn_t_binder(Result(T::*PF)(T1_TN), T *p)
                : m_p(p), m_f(PF)
            {
            }

            Result operator ()(T1t1_TNtn)
            {
                return m_f(m_p FUNC_USE_COMMA_IF t1_tn);
            }
        };

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        mem_fn_t_binder<Result(T::*)(T1_TN)> bind(Result(T::*PF)(T1_TN), T &r)
        {
            return mem_fn_t_binder<Result(T::*)(T1_TN)>(PF, r);
        }

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        const_mem_fn_t_binder<Result(T::*)(T1_TN) const> bind(Result(T::*PF)(T1_TN) const, T &r)
        {
            return const_mem_fn_t_binder<Result(T::*)(T1_TN) const>(PF, r);
        }

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        mem_fn_t_binder<Result(T::*)(T1_TN)> bind(Result(T::*PF)(T1_TN), T *p)
        {
            return mem_fn_t_binder<Result(T::*)(T1_TN)>(PF, p);
        }

        template<class Result, class T FUNC_USE_COMMA_IF CLASST1_CLASSTN>
        const_mem_fn_t_binder<Result(T::*)(T1_TN) const> bind(Result(T::*PF)(T1_TN) const, T *p)
        {
            return const_mem_fn_t_binder<Result(T::*)(T1_TN) const>(PF, p);
        }


        // Function type Result (T1, T2, ..., TN), 0 <= N <= Nmax
        template<class Result FUNC_USE_COMMA_IF  CLASST1_CLASSTN>
        class function <Result (T1_TN)> : public function_base
            USING_FUNCTION
        {
            struct invoker_base
            {
                virtual ~invoker_base(){}
                virtual Result invoke(T1_TN) = 0;
                virtual invoker_base * clone() const = 0;
                virtual void * get_ptr( const type_info & ) const = 0;
                virtual const type_info & get_type_info() const = 0;
            };

            template<class T>
            struct invoker : public invoker_base
            {
                typedef T target_type;
                target_type m_functor_object;

                invoker_base * clone() const
                {
                    return new invoker(*this);
                }

                const type_info & get_type_info() const
                {
                    return typeid(target_type);
                }

                void * get_ptr( const type_info & ti) const
                {
                    if (typeid(target_type) == ti)
                        return (void*) &m_functor_object;
                    return 0;
                }

                Result invoke(T1t1_TNtn)
                {
                    return m_functor_object(t1_tn);
                }

                invoker(const target_type & object)
                    : m_functor_object(object)
                {
                }
            };

            template <class T> void new_invoker( const T &object )
            {
                m_pInvoker = new invoker<T>(object);
            }

            template<class T> void new_invoker( Result (T::*pmFunction) (T1_TN) )
            {
                if (pmFunction == 0)
                    return;

                typedef mem_fn_t<Result (T::*)(T1_TN)> functor;
                m_pInvoker = new invoker< functor >( functor(pmFunction) );
            }

        public:

            typedef Result result_type;

            // [3.7.2.1] construct/copy/destroy
            explicit function() : function_base()
            {
                assert(!*this);
            }

            function(const function& f)
            {
                if (f.m_pInvoker != 0)
                    m_pInvoker = ((invoker_base*)(f.m_pInvoker))->clone();

                assert( (!*this && !f) || (*this && f));
            }

            function(unspecified_null_pointer_type *) : function_base()
            {
                assert(!*this);
            }

            //enable_if here is to disable create function objects of integral types.
            // so is possible to initialize do 0 function f(0);
            template<class F>
            function(const F &f
                ,typename detail::disable_if<is_integral<F> , int>::type = 0
                )
            {
                new_invoker(f);
            }

            function& operator=(const function& f)
            {
                function(f).swap(*this);
                return *this;
            }

            function& operator=(unspecified_null_pointer_type * )
            {
                delete ((invoker_base*)m_pInvoker);
                m_pInvoker = 0;
                assert(!*this);
                return *this;
            }

            template<class F>
            typename detail::disable_if< is_integral<F> , function >::type &
                operator=(const F &f)
            {
                function(f).swap(*this);
                return *this;
            }

            template<class F> function& operator=(reference_wrapper<F> f)
            {
                function(f).swap(*this);
                return *this;
            }

            ~function()
            {
                delete ((invoker_base*)m_pInvoker);
            }

            // [3.7.2.2] function modifiers
            void swap(function& f)
            {
                std::swap(m_pInvoker, f.m_pInvoker);
            }

            // [3.7.2.3] function capacity
            operator bool() const throw()
            {
                return !empty();
            }

            // [3.7.2.4] function invocation
            Result operator()(T1t1_TNtn) const
            {
                if (!*this)
                    throw bad_function_call();

                return ((invoker_base*)m_pInvoker)->invoke(t1_tn);
            }

            // [3.7.2.5] function target access
            const std::type_info& target_type() const
            {
                if (!*this)
                    return typeid(void);
                return ((invoker_base*)m_pInvoker)->get_type_info();
            }

            template <typename T> T* target()
            {
                if (!*this)
                    return 0;
                return (T*)((invoker_base*)m_pInvoker)->get_ptr(typeid(T));
            }

            template <typename T> const T* target() const
            {
                if (!*this)
                    return 0;
                return ((invoker_base*)m_pInvoker)->get_ptr(typeid<T>);
            }

        private:

            // [3.7.2.6] undefined operators
            template<class Function> bool operator==(const function<Function>&);
            template<class Function> bool operator!=(const function<Function>&);
        };

    } // namespace tr1
} // namespace std


#undef CLASST1_CLASSTN
#undef T1_TN
#undef T1t1_TNtn
#undef t1_tn
#undef FUNC_USE_COMMA_IF
#undef USING_FUNCTION
#undef USING_FUNCTIONM
