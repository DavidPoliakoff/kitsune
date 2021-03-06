#include "mltaln.h"
#include "dp.h"

#define DEBUG 0
#define XXXXXXX    0
#define USE_PENALTY_EX  1


#if 1
static void match_calc( float *match, char **s1, char **s2, int i1, int lgth2 ) 
{
	char *seq2 = s2[0];
	int *intptr = amino_dis[(int)s1[0][i1]];

	while( lgth2-- )
		*match++ = intptr[(int)*seq2++];
}
#else
static void match_calc( float *match, char **s1, char **s2, int i1, int lgth2 )
{
	int j;

	for( j=0; j<lgth2; j++ )
		match[j] = amino_dis[(*s1)[i1]][(*s2)[j]];
}
#endif

static float genGtracking( float *lasthorizontalw, float *lastverticalw, 
						char **seq1, char **seq2, 
                        char **mseq1, char **mseq2, 
                        float **cpmx1, float **cpmx2, 
                        int **ijpi, int **ijpj )
{
	int i, j, l, iin, jin, ifi, jfi, lgth1, lgth2, k, limk;
	char gap[] = "-";
	lgth1 = strlen( seq1[0] );
	lgth2 = strlen( seq2[0] );


#if 0
	for( i=0; i<lgth1; i++ ) 
	{
		fprintf( stderr, "lastverticalw[%d] = %f\n", i, lastverticalw[i] );
	}
#endif
 
    for( i=0; i<lgth1+1; i++ ) 
    {
        ijpi[i][0] = -1;
		ijpj[i][0] = -1; // ???
    }
    for( j=0; j<lgth2+1; j++ ) 
    {
		ijpi[0][j] = -1; // ???
        ijpj[0][j] = -1;
    }


	mseq1[0] += lgth1+lgth2;
	*mseq1[0] = 0;
	mseq2[0] += lgth1+lgth2;
	*mseq2[0] = 0;
	iin = lgth1; jin = lgth2;
	limk = lgth1+lgth2 + 1;
	for( k=0; k<limk; k++ ) 
	{
		ifi = ( ijpi[iin][jin] );
		jfi = ( ijpj[iin][jin] );
		l = iin - ifi;
		while( --l ) 
		{
			*--mseq1[0] = seq1[0][ifi+l];
			*--mseq2[0] = *gap;
			k++;
		}
		l= jin - jfi;
		while( --l )
		{
			*--mseq1[0] = *gap;
			*--mseq2[0] = seq2[0][jfi+l];
			k++;
		}
		if( iin <= 0 || jin <= 0 ) break;
		*--mseq1[0] = seq1[0][ifi];
		*--mseq2[0] = seq2[0][jfi];

//		fprintf( stdout, "mseq1 = %s\n", mseq1[0] );
//		fprintf( stdout, "mseq2 = %s\n", mseq2[0] );

		k++;
		iin = ifi; jin = jfi;
	}
	return( 0.0 );
}


float genG__align11( char **seq1, char **seq2, int alloclen )
/* score no keisan no sai motokaraaru gap no atukai ni mondai ga aru */
{
//	int k;
	register int i, j;
	int lasti;                      /* outgap == 0 -> lgth1, outgap == 1 -> lgth1+1 */
	int lgth1, lgth2;
	int resultlen;
	float wm;   /* int ?????? */
	float g;
	float *currentw, *previousw;
	float fpenalty = (float)penalty;
	float fpenalty_OP = (float)penalty_OP;
#if USE_PENALTY_EX
	float fpenalty_ex = (float)penalty_ex;
#endif
#if 1
	float *wtmp;
	int *ijpipt;
	int *ijpjpt;
	float *mjpt, *Mjpt, *prept, *curpt;
	int *mpjpt, *Mpjpt;
#endif
	static float mi, *m;
	static float Mi, *largeM;
	static int **ijpi;
	static int **ijpj;
	static int mpi, *mp;
	static int Mpi, *Mp;
	static float *w1, *w2;
	static float *match;
	static float *initverticalw;    /* kufuu sureba iranai */
	static float *lastverticalw;    /* kufuu sureba iranai */
	static char **mseq1;
	static char **mseq2;
	static char **mseq;
	static float **cpmx1;
	static float **cpmx2;
	static int **intwork;
	static float **floatwork;
	static int orlgth1 = 0, orlgth2 = 0;
	float tbk;
	int tbki, tbkj;

	wm = 0.0;

	if( orlgth1 == 0 )
	{
		mseq1 = AllocateCharMtx( njob, 0 );
		mseq2 = AllocateCharMtx( njob, 0 );
	}


	lgth1 = strlen( seq1[0] );
	lgth2 = strlen( seq2[0] );



	if( lgth1 <= 0 || lgth2 <= 0 )
	{
		fprintf( stderr, "WARNING (g11): lgth1=%d, lgth2=%d\n", lgth1, lgth2 );
	}

	if( lgth1 > orlgth1 || lgth2 > orlgth2 )
	{
		int ll1, ll2;

		if( orlgth1 > 0 && orlgth2 > 0 )
		{
			FreeFloatVec( w1 );
			FreeFloatVec( w2 );
			FreeFloatVec( match );
			FreeFloatVec( initverticalw );
			FreeFloatVec( lastverticalw );

			FreeFloatVec( m );
			FreeIntVec( mp );
			FreeFloatVec( largeM );
			FreeIntVec( Mp );

			FreeCharMtx( mseq );


			FreeFloatMtx( cpmx1 );
			FreeFloatMtx( cpmx2 );

			FreeFloatMtx( floatwork );
			FreeIntMtx( intwork );
		}

		ll1 = MAX( (int)(1.3*lgth1), orlgth1 ) + 100;
		ll2 = MAX( (int)(1.3*lgth2), orlgth2 ) + 100;

#if DEBUG
		fprintf( stderr, "\ntrying to allocate (%d+%d)xn matrices ... ", ll1, ll2 );
#endif

		w1 = AllocateFloatVec( ll2+2 );
		w2 = AllocateFloatVec( ll2+2 );
		match = AllocateFloatVec( ll2+2 );

		initverticalw = AllocateFloatVec( ll1+2 );
		lastverticalw = AllocateFloatVec( ll1+2 );

		m = AllocateFloatVec( ll2+2 );
		mp = AllocateIntVec( ll2+2 );
		largeM = AllocateFloatVec( ll2+2 );
		Mp = AllocateIntVec( ll2+2 );

		mseq = AllocateCharMtx( njob, ll1+ll2 );

		cpmx1 = AllocateFloatMtx( 26, ll1+2 );
		cpmx2 = AllocateFloatMtx( 26, ll2+2 );

		floatwork = AllocateFloatMtx( 26, MAX( ll1, ll2 )+2 ); 
		intwork = AllocateIntMtx( 26, MAX( ll1, ll2 )+2 ); 

#if DEBUG
		fprintf( stderr, "succeeded\n" );
#endif

		orlgth1 = ll1 - 100;
		orlgth2 = ll2 - 100;
	}


	mseq1[0] = mseq[0];
	mseq2[0] = mseq[1];


	if( orlgth1 > commonAlloc1 || orlgth2 > commonAlloc2 )
	{
		int ll1, ll2;

		if( commonAlloc1 && commonAlloc2 )
		{
			FreeIntMtx( commonIP );
			FreeIntMtx( commonJP );
		}

		ll1 = MAX( orlgth1, commonAlloc1 );
		ll2 = MAX( orlgth2, commonAlloc2 );

#if DEBUG
		fprintf( stderr, "\n\ntrying to allocate %dx%d matrices ... ", ll1+1, ll2+1 );
#endif

		commonIP = AllocateIntMtx( ll1+10, ll2+10 );
		commonJP = AllocateIntMtx( ll1+10, ll2+10 );

#if DEBUG
		fprintf( stderr, "succeeded\n\n" );
#endif

		commonAlloc1 = ll1;
		commonAlloc2 = ll2;
	}
	ijpi = commonIP;
	ijpj = commonJP;


#if 0
	for( i=0; i<lgth1; i++ ) 
		fprintf( stderr, "ogcp1[%d]=%f\n", i, ogcp1[i] );
#endif

	currentw = w1;
	previousw = w2;


	match_calc( initverticalw, seq2, seq1, 0, lgth1 );


	match_calc( currentw, seq1, seq2, 0, lgth2 );

	if( outgap == 1 )
	{
		for( i=1; i<lgth1+1; i++ )
		{
			initverticalw[i] += fpenalty;
		}
		for( j=1; j<lgth2+1; j++ )
		{
			currentw[j] += fpenalty;
		}
	}

	for( j=1; j<lgth2+1; ++j ) 
	{
		m[j] = currentw[j-1]; mp[j] = 0;
		largeM[j] = currentw[j-1]; Mp[j] = 0;
	}

	if( lgth2 == 0 )
		lastverticalw[0] = 0.0;               // lgth2==0 no toki error
	else
		lastverticalw[0] = currentw[lgth2-1]; // lgth2==0 no toki error

	if( outgap ) lasti = lgth1+1; else lasti = lgth1;

#if XXXXXXX
fprintf( stderr, "currentw = \n" );
for( i=0; i<lgth1+1; i++ )
{
	fprintf( stderr, "%5.2f ", currentw[i] );
}
fprintf( stderr, "\n" );
fprintf( stderr, "initverticalw = \n" );
for( i=0; i<lgth2+1; i++ )
{
	fprintf( stderr, "%5.2f ", initverticalw[i] );
}
fprintf( stderr, "\n" );
#endif

	for( i=1; i<lasti; i++ )
	{
		wtmp = previousw; 
		previousw = currentw;
		currentw = wtmp;

		previousw[0] = initverticalw[i-1];

		match_calc( currentw, seq1, seq2, i, lgth2 );
#if XXXXXXX
fprintf( stderr, "\n" );
fprintf( stderr, "i=%d\n", i );
fprintf( stderr, "currentw = \n" );
for( j=0; j<lgth2; j++ )
{
	fprintf( stderr, "%5.2f ", currentw[j] );
}
fprintf( stderr, "\n" );
#endif
#if XXXXXXX
fprintf( stderr, "\n" );
fprintf( stderr, "i=%d\n", i );
fprintf( stderr, "currentw = \n" );
for( j=0; j<lgth2; j++ )
{
	fprintf( stderr, "%5.2f ", currentw[j] );
}
fprintf( stderr, "\n" );
#endif
		currentw[0] = initverticalw[i];

		mi = previousw[0]; mpi = 0;
		Mi = previousw[0]; Mpi = 0;

		ijpipt = ijpi[i] + 1;
		ijpjpt = ijpj[i] + 1;
		mjpt = m + 1;
		Mjpt = largeM + 1;
		prept = previousw;
		curpt = currentw + 1;
		mpjpt = mp + 1;
		Mpjpt = Mp + 1;
		tbk = -9999999.9;
		tbki = 0;
		tbkj = 0;
		for( j=1; j<lgth2+1; j++ )
		{
			wm = *prept;
			*ijpipt = i-1;
			*ijpjpt = j-1;

#if 0
			fprintf( stderr, "%5.0f->", wm );
#endif
#if 0
			fprintf( stderr, "%5.0f?", g );
#endif
			if( (g=mi+fpenalty) > wm )
			{
				wm = g;
//				*ijpipt = i - 1; // iranai
				*ijpjpt = mpi;
			}
			if( (g=*prept) >= mi )
			{
				mi = g;
				mpi = j-1;
			}
#if USE_PENALTY_EX
			mi += fpenalty_ex;
#endif

#if 0 
			fprintf( stderr, "%5.0f?", g );
#endif
			if( (g=*mjpt + fpenalty) > wm )
			{
				wm = g;
				*ijpipt = *mpjpt;
				*ijpjpt = j - 1; //IRU!
			}
			if( (g=*prept) >= *mjpt )
			{
				*mjpt = g;
				*mpjpt = i-1;
			}
#if USE_PENALTY_EX
			m[j] += fpenalty_ex;
#endif

#if 1
			g =  tbk + fpenalty_OP;
			if( g > wm )
			{
				wm = g;
				*ijpipt = tbki;
				*ijpjpt = tbkj;
//				fprintf( stderr, "hit! i%d, j%d, ijpi = %d, ijpj = %d\n", i, j, *ijpipt, *ijpjpt );
			}
			if( Mi > tbk )
			{
				tbk = Mi; //error desu.
				tbki = i-1;
				tbkj = Mpi;
			}
			if( *Mjpt > tbk )
			{
				tbk = *Mjpt;
				tbki = *Mpjpt;
				tbkj = j-1;
			}

			if( *prept > *Mjpt )
			{
				*Mjpt = *prept;
				*Mpjpt = i-1;
			}
			if( *prept > Mi )
			{
				Mi = *prept;
				Mpi = j-1;
			}

#endif


#if 0
			fprintf( stderr, "%5.0f ", wm );
#endif
			*curpt++ += wm;
			ijpipt++;
			ijpjpt++;
			mjpt++;
			Mjpt++;
			prept++;
			mpjpt++;
			Mpjpt++;
		}
		lastverticalw[i] = currentw[lgth2-1]; // lgth2==0 no toki error
	}
#if 0
	for( i=0; i<lgth1; i++ )
	{
		for( j=0; j<lgth2; j++ )
		{
			fprintf( stdout, "i,j=%d,%d - ijpi,ijpj=%d,%d\n", i, j, ijpi[i][j], ijpj[i][j] );
		}
	}
	fflush( stdout );
#endif

	genGtracking( currentw, lastverticalw, seq1, seq2, mseq1, mseq2, cpmx1, cpmx2, ijpi, ijpj );

	resultlen = strlen( mseq1[0] );
	if( alloclen < resultlen || resultlen > N )
	{
		fprintf( stderr, "alloclen=%d, resultlen=%d, N=%d\n", alloclen, resultlen, N );
		ErrorExit( "LENGTH OVER!\n" );
	}


	strcpy( seq1[0], mseq1[0] );
	strcpy( seq2[0], mseq2[0] );
#if 0
	fprintf( stderr, "\n" );
	fprintf( stderr, ">\n%s\n", mseq1[0] );
	fprintf( stderr, ">\n%s\n", mseq2[0] );
	fprintf( stderr, "wm = %f\n", wm );
#endif

	return( wm );
}

