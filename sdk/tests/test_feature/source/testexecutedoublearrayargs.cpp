//
// Tests calling of a c-function from a script with double array parameters
//
// Test author: Rob McDonald, inspired by Fredrik Ehnbom
//

#include "utils.h"

#include "../../../add_on/scriptmath/scriptmath.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

static const char * const TESTNAME = "TestExecuteDoubleArrayArgs";

static const char *script =
"int n = 5;                                                       \n"
"string geom_id = 'ABCDEF';                                       \n"
"array<double> uvec, wvec;                                        \n"
"uvec.resize( n );                                                \n"
"wvec.resize( n );                                                \n"
"for( int i = 0 ; i < n ; i++ )                                   \n"
"{                                                                \n"
"    uvec[i] = (i+1)*1.0/(n+1);                                   \n"
"    wvec[i] = (n-i)*1.0/(n+1);                                   \n"
"}                                                                \n"
"array<double> k1vec, k2vec, kavec, kgvec;                        \n"
"cfunction( geom_id, 0, uvec, wvec, k1vec, k2vec, kavec, kgvec ); \n";


static bool testVal = false;
static bool called  = false;

static float  t1 = 0;
static float  t2 = 0;
static double t3 = 0;
static float  t4 = 0;

template < class T >
void FillASArray( vector < T > & in, CScriptArray* out )
{
    out->Resize( in.size() );
    for ( int i = 0 ; i < ( int )in.size() ; i++ )
    {
        out->SetValue( i, &in[i] );
    }
}

template < class T >
void FillSTLVector( CScriptArray* in, vector < T > & out )
{
    out.resize( in->GetSize() );
    for ( int i = 0 ; i < ( int )in->GetSize() ; i++ )
    {
        out[i] = * ( T* ) ( in->At( i ) );
    }
}

void doCalculations( const std::string &geom_id, const int &surf_indx, const vector < double > &us, const vector < double > &ws, vector < double > &k1_out_vec, vector < double > &k2_out_vec, vector < double > &ka_out_vec, vector < double > &kg_out_vec )
{
    called = true;

    k1_out_vec.resize( 0 );
    k2_out_vec.resize( 0 );
    ka_out_vec.resize( 0 );
    kg_out_vec.resize( 0 );

    if ( us.size() == ws.size() )
    {
        k1_out_vec.resize( us.size() );
        k2_out_vec.resize( us.size() );
        ka_out_vec.resize( us.size() );
        kg_out_vec.resize( us.size() );

        for ( int i = 0; i < us.size(); i++ )
        {
            k1_out_vec[i] = us[i];
            k2_out_vec[i] = ws[i];
            ka_out_vec[i] = us[i]*ws[i];
            kg_out_vec[i] = us[i]/ws[i];

            // Store last calculated value for checking later.
            t1 = k1_out_vec[i];
            t2 = k2_out_vec[i];
            t3 = ka_out_vec[i];
            t4 = kg_out_vec[i];
        }
    }

    double eps = 1e-4;
    testVal = (std::abs( t1 - 0.833333 ) < eps ) &&
              (std::abs( t2 - 0.166667 ) < eps ) &&
              (std::abs( t3 - 0.138889 ) < eps ) &&
              (std::abs( t4 - 5.000000 ) < eps );
}

class TestClass
{
public:
    void cfunction( const string &geom_id, const int &surf_indx, CScriptArray *us, CScriptArray *ws, CScriptArray *k1s,
                    CScriptArray *k2s, CScriptArray *kas, CScriptArray *kgs )
    {
        vector < double > in_us;
        FillSTLVector( us, in_us );

        vector < double > in_ws;
        FillSTLVector( ws, in_ws );

        vector < double > out_k1s;
        vector < double > out_k2s;
        vector < double > out_kas;
        vector < double > out_kgs;

        doCalculations( geom_id, surf_indx, in_us, in_ws, out_k1s, out_k2s, out_kas, out_kgs );

        FillASArray( out_k1s, k1s );
        FillASArray( out_k2s, k2s );
        FillASArray( out_kas, kas );
        FillASArray( out_kgs, kgs );
    }
};

bool TestExecuteDoubleArrayArgs()
{
    bool fail = false;

    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

    RegisterScriptArray( engine, true );
    RegisterStdString( engine );
    RegisterScriptMath( engine );

    TestClass tc_instance;

    if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
        PRINTF("\n%s: Test not set up for MAX_PORTABILITY mode.\n\n", TESTNAME);
    else
    {
        int r;
        r = engine->RegisterGlobalFunction( "void cfunction(const string & in geom_id, const int & in surf_indx, array<double>@+ us, array<double>@+ ws, array<double>@+ k1s, array<double>@+ k2s, array<double>@+ kas, array<double>@+ kgs)", asMETHOD( TestClass, cfunction ), asCALL_THISCALL_ASGLOBAL, &tc_instance );
        assert( r >= 0 );
    }

    COutStream out;
    engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

    ExecuteString(engine, script);

    if( !called )
    {
        // failure
        PRINTF("\n%s: cfunction not called from script\n\n", TESTNAME);
        TEST_FAILED;
    }
    else if( !testVal )
    {
        // failure
        PRINTF("\n%s: testVal is not of expected value. Got (%f, %f, %f, %f), expected (%f, %f, %f, %f)\n\n", TESTNAME, t1, t2, t3, t4, 0.833333, 0.166667, 0.138889, 5.000000);
        TEST_FAILED;
    }

    SKIP_ON_MAX_PORT
    {
        called = false;
        testVal = false;
        ExecuteString(engine, script);
        if( !called )
        {
            // failure
            PRINTF("\n%s: cfunction not called from script\n\n", TESTNAME);
            TEST_FAILED;
        }
    }

    engine->Release();

    // Success
    return fail;
}
