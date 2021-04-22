/*****************************************************************************
* File name:  matrix.c
*
* File description: 
*   - matrix mathematics handling functions 
*   - utilizes matrix structure as defined in matrix.h and employs the
*	  matrix block structure as defined in matrix.h
*               
* $Rev: 17451 $
* $Date: 2011-02-09 16:52:56 -0800 (Wed, 09 Feb 2011) $
* $Author: by-dan $
*****************************************************************************/
#include <math.h>
#include "float.h"  
#include "matrix.h"

#define EPSILON 1.0e-12

/***************************************************************************** 
* Module name:  mat_zero
*
* Description:  Sets a matrix to all zeros
*                                   
* Trace: [SDD_MATRIX_ZERO <-- SRC_MAT_ZERO]
*
* Input parameters:  matrixStruct aMat - matrix to zero
*
* Output parameters:  a is full of zeros
*
* Return value:  None
******************************************************************************/
void mat_zero( matrixStruct aMat ) {
    int i, mnMat;                   
    mnMat = aMat.m * aMat.n;
    for( i = 0; i<mnMat; i++ )  {
        *(aMat.Ptr + i) = 0.0;  
    }
} /* end mat_zero */  

/*****************************************************************************
* Module name:  mat_make_identity
*
* Description:  to set a matrix to the identity matrix
*
* Trace: [SDD_MATRIX_IDENTITY <-- SRC_MAT_MAKE_IDENTITY]
*                                   
* Input parameters:  matrixStruct aMat - matrix to make identity
*
* Output parameters:  if the # of rows of a does not equal the 
*                     # of cols of a, then do not create the matrix
*                     and return
*                     else a is all zeros except diagonal which is ones
*                     (identity matrix)
******************************************************************************/
void mat_make_identity( matrixStruct aMat ) {
    int i, j;

    if(aMat.m != aMat.n) return;
    
    for( i = 0; i < aMat.m; i++ ) {
        for( j = 0; j < aMat.n; j++ )  {
            *(aMat.Ptr + i*aMat.n + j) = (i == j) ? 1.0 : 0.0; 
    	} 
    }
}  /* end function mat_make_identity */

/*****************************************************************************
* Module name:  mat_subtract
*
* Description:  to create the difference matrix
*               c= a-b
*
* Trace: 
*	[SDD_MATRIX_SUBTRACT <-- SRC_MAT_SUBTRACT]
*	[SDD_MATRIX_SUBTRACT_ERROR <-- SRC_MAT_SUBTRACT]
*                                
* Input parameters:  - matrixStruct aMat - matrix to subtract from
*                    - matrixStruct bMat - matrix to subract
*                    - matrixStruct *c - resulting difference matrix
*                    - int *key - 0 if successful, 1 if not
*
* Output parameters:  if size of a != size of b, return set key =1
*                     if size of *c != size of a, return set key =1
*                     otherwise *c = a-b for each element, key = 0
*
* Return value:  None  
******************************************************************************/
void mat_subtract( matrixStruct aMat, matrixStruct bMat, matrixStruct *cMat, 
                  int *key )
{
	int i, mnMat;
    if( aMat.m != bMat.m || aMat.n != bMat.n ) {
        *key = 1;
        return;
    }
    if( cMat->m != aMat.m || cMat->n != aMat.n ) {
        *key = 1;
        return;
    }

    mnMat = aMat.m * aMat.n;

    for( i = 0; i < mnMat; i++ )   {
    	*(cMat->Ptr + i) = *(aMat.Ptr + i) - *(bMat.Ptr + i);
    }
    *key = 0;
} /* end function mat_subtract */

/*****************************************************************************
* Module name:  mat_add
*
* Description:  to create a matric that is the sum of two others
*               c= a+b
* 
* Trace: 
*	[SDD_MATRIX_ADD <-- SRC_MAT_ADD]
*	[SDD_MATRIX_ADD_ERROR <-- SRC_MAT_ADD]
*                                
* Input parameters:  - matrixStruct aMat - matrix to add
*                    - matrixStruct bMat - matrix to add
*                    - matrixStruct *c - resulting sum matrix
*                    - int *key - 0 if successful, 1 if not
*
* Output parameters:  if size of a != size of b, return set key =1
*                     if size of *c != size of a, return set key =1
*                     otherwise *c = a+b for each element, key = 0
*
* Return value:  None   
******************************************************************************/
void mat_add( matrixStruct aMat, matrixStruct bMat, matrixStruct *cMat, 
             int *key )
{
	int i, mnMat = aMat.m * aMat.n;

    if( aMat.m != bMat.m || aMat.n != bMat.n){
        *key = 1;
        return;
    }

    if( cMat->m != aMat.m || cMat->n != aMat.n ) {
        *key = 1;
        return;
    }

    for( i = 0; i < mnMat; i++ )
    	*((*cMat).Ptr+i) = *(aMat.Ptr+i) + *(bMat.Ptr+i);
                   
    *key = 0;
} /* end function mat_add */

/*****************************************************************************
* Module name:  mat_multiply
*
* Description:  to create a matric that is the result of multiplying two others
*               c= a*b
*               note that this is not commutative (a*b != b*a)
*
*               To calculate the c[i][j] all of the ith row of the a matrix 
*               are multipled by the jth column of the b matrix and the results 
*               are summed
*
* Trace: 
*	[SDD_MATRIX_MULTIPLY <-- SRC_MAT_MULTIPLY]
*	[SDD_MATRIX_MULTIPLY_ERROR <-- SRC_MAT_MULTIPLY]
*
* Input parameters:  - matrixStruct aMat - matrix to multiply, size (m,n)
*                    - matrixStruct bMat - matrix to multiply, size (n,p)
*                    - matrixStruct *c - resulting matrix,  size (m,p)
*                    - int *key - 0 if successful, 1 if not
*
* Output parameters:  if the number of rows in a != number of cols in b, 
*                     then return key = 1, no multiplication is performed
*                     if the number of rows in c != number of rows in a,
*                     or if the number of cols in c != number of cols in b, 
*                     then return key = 1, no multiplication is performed 
*                     else key = 0, c = a*b
*
* Return value:  None
******************************************************************************/
void mat_multiply( matrixStruct aMat, matrixStruct bMat, matrixStruct *cMat, 
                  int *key )
{
    double dummy=0;
    int i, j, k;

    if( aMat.n != bMat.m ){
        *key = 1;
        return;
    }

    if( cMat->m != aMat.m || cMat->n != bMat.n ) {
        *key = 1;
        return;
    }

    for( i = 0; i < aMat.m; i++ ) {
    	for( j = 0; j < bMat.n; j++ ) {
        	dummy = 0.0;
            for( k = 0; k < aMat.n; k++ )
                dummy += *(aMat.Ptr+i*aMat.n+k) * *(bMat.Ptr+j+bMat.n*k);
            *(cMat->Ptr+i*bMat.n+j) = dummy;
        }  /* end for j->bMat.n */
    }  /* end for i->aMat.m */
    *key = 0;
} /* end function mat_multiply */

/*****************************************************************************
* Module name:  mat_move
*
* Description:  to copy a to b
*
* Trace: [SDD_MATRIX_MOVE <-- SRC_MAT_MOVE]
* 
* Input parameters:  - matrixStruct aMat - master matrix
*                    - matrixStruct bMat - matrix to receive a
*
* Output parameters:  if the # of rows of b does not equal the 
*                     # of cols of a, or the # of cols of b does not 
*                     equal the # of cols of a, then do not create 
*                     the matrix and return
*                     else *b = a
*
* Return value:  None
******************************************************************************/
void mat_move( matrixStruct aMat, matrixStruct *bMat ) {

    double  *ptrA,*ptrB;
    int i,mnMat;

    if( bMat->m != aMat.m || bMat->n != aMat.n ) {
    	return;
    }
    
    mnMat = aMat.m*aMat.n;
    ptrA = aMat.Ptr;
    ptrB = (*bMat).Ptr;

    for(i=0; i<mnMat; i++)    {                
        *ptrB++ = *ptrA++;
    }
    
    bMat->m = aMat.m;
    bMat->n = aMat.n;
}  /* end function mat_move */

/*****************************************************************************
* Module name:  mat_multiply_invert
*
* Description:  matrix inversion and multiply routine
*               input matrices a[n][m] and b[m][m]
*               After Calculation, output a is such that
*               a = a*inv(b),
*
* Trace: 
*	[SDD_MATRIX_MULTIPLY_INVERT <-- SRC_MAT_MULTIPLY_INVERT]
*	[SDD_MATRIX_MULTIPLY_INVERT_ERROR <-- SRC_MAT_MULTIPLY_INVERT]
*
* Input parameters:  - matrixStruct aMat - master matrix, size n,m
*                    - matrixStruct bMat - matrix to invert, size m,m
*                    - int * key - error check
*
* Output parameters:  If b is not a square matrix (row=cols) OR 
*                     the size of a rows != size of b cols then *key = 1.
*					  If matrix b contains elements that are too small,
*					  then *key = 1, as a warning for an ill conditioned
*					  matrix.  Otherwise, *key = 0 and a is changed so that 
*					  a = a*inv(b)
*
* Return value:  None
******************************************************************************/
void mat_multiply_invert( matrixStruct aMat, matrixStruct bMat, int *key ) 
{
    int maxCol, i, j, k, m,n;    
    double invPivot, max, tmp;

    m = bMat.m;
    n = aMat.m; 

    if( bMat.m != bMat.n || bMat.m != aMat.n ) {
        *key = 1;
        return;
    }

    for( i = 0; i < m - 1; i++ ) {
        maxCol = i;
        max = fabs(*(bMat.Ptr+i*m+i));
        for( j = i + 1; j < m; j++ ) {
                if( ( tmp = fabs( *(bMat.Ptr+i*m+j) ) ) > max ) {
                max = tmp;
                maxCol = j;
            }
        }
        /*                   */
        /* Partial Pivoting  */
        /*                   */
        if( maxCol != i ) {
                for( j = i; j < m; j++ ) {
                tmp = *(bMat.Ptr+j*m+i);
                *(bMat.Ptr+j*m+i) = *(bMat.Ptr+j*m+maxCol);
                *(bMat.Ptr+j*m+maxCol) = tmp;
            }
            for( j = 0; j < n; j++ ) {
                tmp = *(aMat.Ptr+j*m+i);
                *(aMat.Ptr+j*m+i) = *(aMat.Ptr+j*m+maxCol);
                *(aMat.Ptr+j*m+maxCol) = tmp;
            }
        }

        if( fabs( *(bMat.Ptr+i*m+i) ) < EPSILON ) {
            *key = 1;
            return;
        }

        invPivot = 1 / ( *(bMat.Ptr+i*m+i) );
        for( j = i; j < m; j++ )
                *(bMat.Ptr+j*m+i) *= invPivot;
        for( j = 0; j < n; j++ )
                *(aMat.Ptr+j*m+i) *= invPivot;

        for( j = 0; j < m; j++ ) {
                if( j != i ) {
                tmp = *(bMat.Ptr+i*m+j);
                for( k = i; k < m; k++ )
                        *(bMat.Ptr+k*m+j) -= tmp * *(bMat.Ptr+k*m+i);
                for( k = 0; k < n; k++ )
                        *(aMat.Ptr+k*m+j) -= tmp * *(aMat.Ptr+k*m+i);
            }
        }
    }

    if( fabs( *(bMat.Ptr+(m-1)*m+m-1) ) < EPSILON ) {
        *key = 1;
        return;
    }

    invPivot = 1 / ( *(bMat.Ptr+(m-1) * m + m - 1) );
    for( j = 0; j < n; j++ ) {
        *(aMat.Ptr+m*j+m-1) *= invPivot;
    }
    
    for( j = 0; j < m - 1; j++ ) {
        tmp = *(bMat.Ptr+(m-1)*m+j);
        for( k = 0; k < n; k++ )  {
                *(aMat.Ptr+k*m+j) -= tmp * *(aMat.Ptr+k*m+m-1);  
        }
    }
    *key = 0;
} /* end function mat_multiply_invert */

/*****************************************************************************
* Module name:  mat_multiply_constant
*
* Description:  multiply by a constant
*
* Trace: [SDD_MATRIX_MULTIPLY_CONSTANT <-- SRC_MAT_MULTIPLY_CONSTANT]
*
* Input parameters:  - double c - constant multiplicand
*                    - matrixStruct aMat - matic to multiply by constant
*                    - matrixStruct bMat - matrix to receive multiplication 
*
* Output parameters:  no errors checked
*                     b= cA
*
* Return value:  None
******************************************************************************/
void mat_multiply_constant( double cInput, matrixStruct aMat, 
                          matrixStruct *bMat ) 
{
    double *ptrA,*ptrB;
    int i,mnMat;

    mnMat = aMat.m*aMat.n;
    ptrA = aMat.Ptr;
    ptrB = (*bMat).Ptr;

    for(i=0; i<mnMat; i++) {
        *bMat->Ptr++ = cInput* *aMat.Ptr++;
    }
    
    aMat.Ptr    = ptrA;
    bMat->Ptr = ptrB;
}  /* end function mat_multipy_constant */

/*****************************************************************************
* Module name:  mat_transpose
*
* Description:  transpose a matrix
*
* Trace: [SDD_MATRIX_TRANSPOSE <-- SRC_MAT_TRANSPOSE]
*
* Input parameters:  - matrixStruct aMat - matix to transpose
*                    - matrixStruct *bMat - transposed matrix 
*
* Output parameters:  no errors checked
*                     b=transpose of A
*
* Return value:  None
******************************************************************************/
void mat_transpose( matrixStruct aMat, matrixStruct *bMat ) {
	int i, j;

    for( i = 0; i < aMat.m; i++ ) {
    	for( j = 0; j < aMat.n; j++ )     {
        	*(bMat->Ptr + aMat.m*j + i) = *(aMat.Ptr + aMat.n*i + j);  
        }
    }
} /* end function mat_transpose */

/*****************************************************************************
* Module name:  mat_move_matblock
*
* Description:  to copy block matrix A to block matrix B 
* 
* Trace: [SDD_MATRIX_BLOCK_MOVE <-- SRC_MAT_MOVE_MATBLOCK]
* 
* Input parameters:  -matrixBlockStruct aMat - master matrix
*                    -matrixBlockStruct bMat - matrix to receive A
*
* Output parameters:  if the # of rows of the matrix block A.M does not equal
*                     the # of rows of the matrix block B.M or if the # of 
*                     cols of the matrix block A.M does not equal the # of 
*                     cols of the matrix block B.M then do not perform the move,
*                     and return
*                     else 
*                     *B = A
* Return value:  None
******************************************************************************/
void mat_move_matblock(matrixBlockStruct aMat,matrixBlockStruct *bMat)
{
        double *Ptr1,*Ptr2;
        int i,mnMat;

        if(bMat->M.m != aMat.M.m || bMat->M.n != aMat.M.n) {
        	return; 
        }
        
        mnMat = aMat.M.m*aMat.M.n;
        Ptr1 = aMat.M.Ptr;
        Ptr2 = bMat->M.Ptr;

        for(i=0; i<mnMat; i++)    {
                *Ptr2++ = *Ptr1++; 
                
        }
}/* end function mat_move_matblock */

/*****************************************************************************
* Module name:  mat_move_block
*
* Description:  to copy a matrix a into the block position (rowblk, colblk)
*               of block matrix b
*
* Trace: [SDD_MATRIX_BLOCK_MOVE_MAT_INTO_BLOCK <-- SRC_MAT_MOVE_BLOCK]
* 
* Input parameters:  -matrixStruct aMat - master matrix
*                    -matrixBlockStruct bMat - matrix to receive a in given block
*                    -int rowblk - starting row of block 
*                    -int colblk - starting column of block
*
* Output parameters:  no errors checked
*                     b[rowblk][colblk] = a[0][0], block copied
*
* Return value:  None
******************************************************************************/
void mat_move_block( matrixStruct aMat, matrixBlockStruct bMat, 
                   int rowblk, int colblk ) {
	int i, j, n, colStart, rowStart, colWidth, rowWidth;
    double *ptr;

    n = bMat.M.n;
    rowStart = 0;
    rowWidth = 0;
    if( rowblk == 0 )    {
    	rowWidth = bMat.M.m;
    }    else{
    	for( i = 0; i < rowblk; i++ ) {
        	rowStart += rowWidth;
            rowWidth = *(bMat.mSizePtr + i);
        }
    }
    colStart = 0;
    colWidth = 0;
    if( colblk == 0 )
    	colWidth = n;
    else{
    	for( i = 0; i < colblk; i++ ) {
        	colStart += colWidth;
            colWidth = *(bMat.nSizePtr + i);
        }
    }
    ptr = bMat.M.Ptr + rowStart*n + colStart;
    for( i = 0; i < rowWidth; i++ ) {
    	for( j = 0; j < colWidth; j++ )    {
        	*(ptr+n*i+j) = *(aMat.Ptr+aMat.n*i+j); 
        }
    }
} /* end function mat_move_block */

/*****************************************************************************
* Module name:  mat_transpose_block
*
* Description:  transpose a block matrix
*
* Trace: [SDD_MATRIX_BLOCK_TRANSPOSE <-- SRC_MAT_TRANSPOSE_BLOCK]
*
* Input parameters:  - matrixBlockStruct *a - matix to transpose
*                    - matrixBlockStruct *bMat - transposed matrix 
*
* Output parameters:  no errors checked
*                     b=transpose of a
*
* Return value:  None
******************************************************************************/
void mat_transpose_block(matrixBlockStruct aMat, matrixBlockStruct *bMat) 
{
	int i, j, m, n, *ptrIA, *ptrIB;
    double *ptrA, *ptrB;

    m = aMat.M.m;
    n = aMat.M.n;
    bMat->M.m = n;
    bMat->M.n = m;
    ptrA = aMat.M.Ptr;
    ptrB = bMat->M.Ptr;

    for( i = 0; i < m; i++ ) {
    	for( j = 0; j < n; j++ ) {
        	*(ptrB + m*j + i) = *(ptrA + n*i + j);     
        }
    }
    m = aMat.mtype;
    n = aMat.ntype;
    bMat->ntype = m;
    bMat->mtype = n;
    ptrIA = aMat.typePtr;
    ptrIB = bMat->typePtr;
    for( i = 0; i < m; i++ ) {
    	*(bMat->nSizePtr + i) = *(aMat.mSizePtr + i);
        for( j = 0; j < n; j++ )   {
        	*(ptrIB + m*j + i) = *(ptrIA + n*i + j);  
        }
    }
    for( i = 0; i < n; i++ )     {
    	*(bMat->mSizePtr + i) = *(aMat.nSizePtr + i);   
    }
} /* end function mat_transpose_block */

/*****************************************************************************
* Module name:  mat_multiply_block
*
* Description:  *c=a*B
*               sym > 0 if c is symmetric
*
* Trace:
* [SDD_MATRIX_BLOCK_MULTIPLY <-- SRC_MAT_MULTIPLY_BLOCK]
* [SDD_MATRIX_BLOCK_MULTIPLY_ERROR <-- SRC_MAT_MULTIPLY_BLOCK]
* [SDD_MATRIX_BLOCK_MULTIPLY_TYPE0 <-- SRC_MAT_MULTIPLY_BLOCK]
* [SDD_MATRIX_BLOCK_MULTIPLY_TYPE1 <-- SRC_MAT_MULTIPLY_BLOCK]
* [SDD_MATRIX_BLOCK_MULTIPLY_TYPE2 <-- SRC_MAT_MULTIPLY_BLOCK]
* [SDD_MATRIX_BLOCK_MULTIPLY_TYPE3 <-- SRC_MAT_MULTIPLY_BLOCK]
* [SDD_MATRIX_BLOCK_MULTIPLY_SYMMETRIC <-- SRC_MAT_MULTIPLY_BLOCK]
*
* Input parameters:  - matrixBlockStruct a
*                    - matrixBlockStruct bMat
*                    - matrixBlockStruct *c
*                    - int sym - control flag 
*                    - int *key - error flag
*
* Output parameters:  if sizes are wrong, key = 1
*                     otherwise key = 0 and
*                     *c = a*b                     
*                     sym > 0  if c is symmetric   
*
* Return value:  None
******************************************************************************/
void mat_multiply_block( matrixBlockStruct aMat, matrixBlockStruct bMat, 
                      matrixBlockStruct *cMat, int sym, int *key ) 
{
    int i, j, k, l, m, n,
    mA,mB,mC,nA,nB,nC,mAblk,mBblk,nAblk,nBblk,
    colA,colB,colC,sumcolA,sumcolB,sumcolC,
    rowA,rowB,rowC,sumrowA,sumrowB,sumrowC,
    startA,startB,startC,
    typeA,typeB,typeC,typeTemp;

    double temp, *ptrA, *ptrB, *ptrC, *ptr;

    mA = aMat.M.m;
    mB = bMat.M.m;
    nA = aMat.M.n;
    nB = bMat.M.n;
    mAblk = aMat.mtype;
    mBblk = bMat.mtype;
    nAblk = aMat.ntype;
    nBblk = bMat.ntype;

    if( nA != mB ) {
        *key = 1;
        return;
    }

    for( i = 0; i < nAblk; i++ ) {
    	if( *(aMat.nSizePtr + i) != *(bMat.mSizePtr + i) ) {
            *key = 1;
            return;
        }
    }
    cMat->M.m = mA;
    cMat->M.n = nB;
    mC = mA;
    nC = nB;
    cMat->mtype = mAblk;
    cMat->ntype = nBblk;
    for( i = 0; i < nBblk; i++ ) {
    	*(cMat->nSizePtr + i) = *(bMat.nSizePtr + i);
    }
    for( i = 0; i < mAblk; i++ ) {
    	*(cMat->mSizePtr + i) = *(aMat.mSizePtr + i);
    }

    rowA = 0;
    sumrowA = 0;

    for( i = 0; i < mAblk; i++ ) {
    	sumrowA += rowA;
        sumrowC = sumrowA;
        rowA = *(aMat.mSizePtr + i);
        *(cMat->mSizePtr + i) = rowA;
        rowC = rowA;
        colB = 0;
        sumcolB = 0;

        for( j = 0; j < nBblk; j++ ) {
        	sumcolB += colB;
            sumcolC = sumcolB;

            colB = *(bMat.nSizePtr + j);
            colC = colB;
            colA = 0;
            sumcolA = 0;
            rowB = 0;
            sumrowB = 0;
            startC = sumrowC * nC + sumcolC;
            ptrC = cMat->M.Ptr + startC;

            if( ( i > j ) && ( sym >0 ) ) {
            	*(cMat->typePtr+i*cMat->ntype+j) = *(cMat->typePtr+j*cMat->ntype+i);
                ptr = cMat->M.Ptr + sumrowC + sumcolC*nC;
                for( l = 0; l < rowC; l++ ) {
                	for( m = 0; m < colC; m++ )  {
                    	*(ptrC + l*nC + m) = *(ptr + m*nC + l); 
                    }
                }
            } else {
            	typeC = 0;
                for( l = 0; l < rowC; l++ ) {
                	for( m = 0; m < colC; m++ ) {
                    	*(ptrC+l*nC+m) = 0.0;  
                    }
                }
                for( k = 0; k < nAblk; k++ ) {
                	typeA = *(aMat.typePtr + i*nAblk + k);
                    typeB = *(bMat.typePtr + k*nBblk + j);
                    typeTemp = 0;
                    sumcolA += colA;
                    sumrowB += rowB;
                    startA = sumrowA*nA + sumcolA;
                    startB = sumrowB*nB + sumcolB;
                    ptrA = aMat.M.Ptr + startA;
                    ptrB = bMat.M.Ptr + startB;
                    colA = *(aMat.nSizePtr + k);
                    rowB = *(bMat.mSizePtr + k);

                    if( typeA == 1 ) {
                    	if( typeB == 1 || typeB ==2 ) {
                        	for( l = 0; l < colA; l++ )  {
                            	*(ptrC+(nC+1)*l) += *(ptrB+(nB+1)*l); 
                            }
                            typeTemp = typeB;
                        } else if( typeB == 3 ) {
                        	for( l = 0; l < rowB; l++ ) {
                            	for( m = 0; m < colB; m++ ) {
                                	*(ptrC+nC*l+m) += *(ptrB+nB*l+m); 
                                }
                        	}
                        	typeTemp = 3;
                    	}
                	} else if( typeA == 2 ) {
                		if( typeB == 1 ) {
                    		for( l = 0; l < colA; l++ ) {
                        		*(ptrC+(nC+1)*l) += *(ptrA+(nA+1)*l);
                        	}
                        	typeTemp = 2;
                    	} else if( typeB == 2 ) {
                    		for( l = 0; l < colA; l++ )  {
                        		*(ptrC+(nC+1)*l) += *(ptrA+(nA+1)*l)* 
                        		                      *(ptrB+(nB+1)*l);
                        	}
                        	typeTemp = 2;
                    	} else if( typeB == 3 ) {
                    		for( l = 0; l < rowB; l++ ) {
                        		temp = *(ptrA+(nA+1)*l);
                            	for( m = 0; m < colB; m++ )  {
                            		*(ptrC+nC*l+m) += *(ptrB+nB*l+m)*temp;
                        	    }
                        	}
                        	typeTemp = 3;
                    	}
                	} else if( typeA == 3 ) {
                		if( typeB == 1 ) {
                    		for( l = 0; l < rowA; l++ ) {
                        		for( m = 0; m < colA; m++ )   {
                            		*(ptrC+nC*l+m) += *(ptrA+nA*l+m);  
                            	}
                        	}
                        	typeTemp = 3;
                    	} else if( typeB == 2 ) {
                    		for( l = 0; l < colA; l++ ) {
                        		temp = *(ptrB+(nB+1)*l);
                            	for( m = 0; m < rowA; m++ )  {
                            		*(ptrC+nC*m+l) += *(ptrA+nA*m+l)*temp;  
                            	}
                        	}
                        	typeTemp = 3;
                    	} else if( typeB == 3 ) {
                    		for( l = 0; l < rowA; l++ ) {
                        		for( m = 0; m < colB; m++ ) {
                            		temp = 0.0;
                                	for( n = 0; n < colA; n++ )  {
                                		temp += *(ptrA+nA*l+n)* 
                                		          *(ptrB+nB*n+m);
                                    }
                                    *(ptrC+nC*l+m) += temp;
                            	}
                        	}
                        	typeTemp = 3;
                    	}
                	}
                	if( typeC < typeTemp )       {
                		typeC = typeTemp;
                	}
            	}
            	*(cMat->typePtr + i*cMat->ntype + j) = typeC;
        	}
    	}
	}
	*key = 0;
} /* end function mat_multiply_block */

/****************************************************************************** 
* Function  name:  max_value_in_vector
*
* Description:  Finds the maximum in the given vector, returns
*				its value and its location
*                                   
* Trace:  [SDD_MATRIX_VECTOR_MAX <-- SRC_MAX_VALUE_IN_VECTOR] 
*
* Input parameters:  matrixStruct aMat - vector
*
* Output parameters:  double *val - the value of the maximum entry in 
*                                   matrixStruct aMat
*					  int *index - the index in matrixStruct aMat of its max value
*
* Return value:  None
******************************************************************************/
void max_value_in_vector(matrixStruct aMat,double *val,int *index)
{
    int i,n;

    n = aMat.n;
    if(aMat.m > aMat.n)
        n = aMat.m;

    *val = *(aMat.Ptr);
    *index = 1;
    for(i=1; i<n; i++){
        if(*(aMat.Ptr+i) > *val){
            *val = *(aMat.Ptr+i);
            *index = i+1;
        }
    }
}  /* end max_value_in_vector */
