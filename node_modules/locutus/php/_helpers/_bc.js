'use strict';

module.exports = function _bc() {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/_helpers/_bc
  // original by: lmeyrick (https://sourceforge.net/projects/bcmath-js/)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //   example 1: var $bc = _bc()
  //   example 1: var $result = $bc.PLUS
  //   returns 1: '+'

  /**
   * BC Math Library for Javascript
   * Ported from the PHP5 bcmath extension source code,
   * which uses the Libbcmath package...
   *    Copyright (C) 1991, 1992, 1993, 1994, 1997 Free Software Foundation, Inc.
   *    Copyright (C) 2000 Philip A. Nelson
   *     The Free Software Foundation, Inc.
   *     59 Temple Place, Suite 330
   *     Boston, MA 02111-1307 USA.
   *      e-mail:  philnelson@acm.org
   *     us-mail:  Philip A. Nelson
   *               Computer Science Department, 9062
   *               Western Washington University
   *               Bellingham, WA 98226-9062
   *
   * bcmath-js homepage:
   *
   * This code is covered under the LGPL licence, and can be used however you want :)
   * Be kind and share any decent code changes.
   */

  /**
   * Binary Calculator (BC) Arbitrary Precision Mathematics Lib v0.10  (LGPL)
   * Copy of Libbcmath included in PHP5 src
   *
   * Note: this is just the shared library file and does not include the php-style functions.
   *       use bcmath{-min}.js for functions like bcadd, bcsub etc.
   *
   * Feel free to use how-ever you want, just email any bug-fixes/improvements
   * to the sourceforge project:
   *
   *
   * Ported from the PHP5 bcmath extension source code,
   * which uses the Libbcmath package...
   *    Copyright (C) 1991, 1992, 1993, 1994, 1997 Free Software Foundation, Inc.
   *    Copyright (C) 2000 Philip A. Nelson
   *     The Free Software Foundation, Inc.
   *     59 Temple Place, Suite 330
   *     Boston, MA 02111-1307 USA.
   *      e-mail:  philnelson@acm.org
   *     us-mail:  Philip A. Nelson
   *               Computer Science Department, 9062
   *               Western Washington University
   *               Bellingham, WA 98226-9062
   */

  var Libbcmath = {
    PLUS: '+',
    MINUS: '-',
    BASE: 10,
    // must be 10 (for now)
    scale: 0,
    // default scale
    /**
     * Basic number structure
     */
    bc_num: function bc_num() {
      this.n_sign = null; // sign
      this.n_len = null; // (int) The number of digits before the decimal point.
      this.n_scale = null; // (int) The number of digits after the decimal point.
      // this.n_refs = null; // (int) The number of pointers to this number.
      // this.n_text = null; // ?? Linked list for available list.
      this.n_value = null; // array as value, where 1.23 = [1,2,3]
      this.toString = function () {
        var r, tmp;
        tmp = this.n_value.join('');

        // add minus sign (if applicable) then add the integer part
        r = (this.n_sign === Libbcmath.PLUS ? '' : this.n_sign) + tmp.substr(0, this.n_len);

        // if decimal places, add a . and the decimal part
        if (this.n_scale > 0) {
          r += '.' + tmp.substr(this.n_len, this.n_scale);
        }
        return r;
      };
    },

    /**
     * Base add function
     *
     //  Here is the full add routine that takes care of negative numbers.
     //  N1 is added to N2 and the result placed into RESULT.  SCALE_MIN
     //  is the minimum scale for the result.
     *
     * @param {bc_num} n1
     * @param {bc_num} n2
     * @param {int} scaleMin
     * @return bc_num
     */
    bc_add: function bc_add(n1, n2, scaleMin) {
      var sum, cmpRes, resScale;

      if (n1.n_sign === n2.n_sign) {
        sum = Libbcmath._bc_do_add(n1, n2, scaleMin);
        sum.n_sign = n1.n_sign;
      } else {
        // subtraction must be done.
        cmpRes = Libbcmath._bc_do_compare(n1, n2, false, false); // Compare magnitudes.
        switch (cmpRes) {
          case -1:
            // n1 is less than n2, subtract n1 from n2.
            sum = Libbcmath._bc_do_sub(n2, n1, scaleMin);
            sum.n_sign = n2.n_sign;
            break;

          case 0:
            // They are equal! return zero with the correct scale!
            resScale = Libbcmath.MAX(scaleMin, Libbcmath.MAX(n1.n_scale, n2.n_scale));
            sum = Libbcmath.bc_new_num(1, resScale);
            Libbcmath.memset(sum.n_value, 0, 0, resScale + 1);
            break;

          case 1:
            // n2 is less than n1, subtract n2 from n1.
            sum = Libbcmath._bc_do_sub(n1, n2, scaleMin);
            sum.n_sign = n1.n_sign;
        }
      }
      return sum;
    },

    /**
     * This is the "user callable" routine to compare numbers N1 and N2.
     * @param {bc_num} n1
     * @param {bc_num} n2
     * @return int -1, 0, 1  (n1 < n2, ===, n1 > n2)
     */
    bc_compare: function bc_compare(n1, n2) {
      return Libbcmath._bc_do_compare(n1, n2, true, false);
    },

    _one_mult: function _one_mult(num, nPtr, size, digit, result, rPtr) {
      var carry, value; // int
      var nptr, rptr; // int pointers
      if (digit === 0) {
        Libbcmath.memset(result, 0, 0, size); // memset (result, 0, size);
      } else {
        if (digit === 1) {
          Libbcmath.memcpy(result, rPtr, num, nPtr, size); // memcpy (result, num, size);
        } else {
          // Initialize
          nptr = nPtr + size - 1; // nptr = (unsigned char *) (num+size-1);
          rptr = rPtr + size - 1; // rptr = (unsigned char *) (result+size-1);
          carry = 0;

          while (size-- > 0) {
            value = num[nptr--] * digit + carry; // value = *nptr-- * digit + carry;
            result[rptr--] = value % Libbcmath.BASE; // @CHECK cint //*rptr-- = value % BASE;
            carry = Math.floor(value / Libbcmath.BASE); // @CHECK cint //carry = value / BASE;
          }

          if (carry !== 0) {
            result[rptr] = carry;
          }
        }
      }
    },

    bc_divide: function bc_divide(n1, n2, scale) {
      // var quot // bc_num return
      var qval; // bc_num
      var num1, num2; // string
      var ptr1, ptr2, n2ptr, qptr; // int pointers
      var scale1, val; // int
      var len1, len2, scale2, qdigits, extra, count; // int
      var qdig, qguess, borrow, carry; // int
      var mval; // string
      var zero; // char
      var norm; // int
      // var ptrs // return object from one_mul
      // Test for divide by zero. (return failure)
      if (Libbcmath.bc_is_zero(n2)) {
        return -1;
      }

      // Test for zero divide by anything (return zero)
      if (Libbcmath.bc_is_zero(n1)) {
        return Libbcmath.bc_new_num(1, scale);
      }

      /* Test for n1 equals n2 (return 1 as n1 nor n2 are zero)
        if (Libbcmath.bc_compare(n1, n2, Libbcmath.MAX(n1.n_scale, n2.n_scale)) === 0) {
          quot=Libbcmath.bc_new_num(1, scale);
          quot.n_value[0] = 1;
          return quot;
        }
      */

      // Test for divide by 1.  If it is we must truncate.
      // @todo: check where scale > 0 too.. can't see why not
      // (ie bc_is_zero - add bc_is_one function)
      if (n2.n_scale === 0) {
        if (n2.n_len === 1 && n2.n_value[0] === 1) {
          qval = Libbcmath.bc_new_num(n1.n_len, scale); // qval = bc_new_num (n1->n_len, scale);
          qval.n_sign = n1.n_sign === n2.n_sign ? Libbcmath.PLUS : Libbcmath.MINUS;
          // memset (&qval->n_value[n1->n_len],0,scale):
          Libbcmath.memset(qval.n_value, n1.n_len, 0, scale);
          // memcpy (qval->n_value, n1->n_value, n1->n_len + MIN(n1->n_scale,scale)):
          Libbcmath.memcpy(qval.n_value, 0, n1.n_value, 0, n1.n_len + Libbcmath.MIN(n1.n_scale, scale));
          // can we return here? not in c src, but can't see why-not.
          // return qval;
        }
      }

      /* Set up the divide.  Move the decimal point on n1 by n2's scale.
       Remember, zeros on the end of num2 are wasted effort for dividing. */
      scale2 = n2.n_scale; // scale2 = n2->n_scale;
      n2ptr = n2.n_len + scale2 - 1; // n2ptr = (unsigned char *) n2.n_value+n2.n_len+scale2-1;
      while (scale2 > 0 && n2.n_value[n2ptr--] === 0) {
        scale2--;
      }

      len1 = n1.n_len + scale2;
      scale1 = n1.n_scale - scale2;
      if (scale1 < scale) {
        extra = scale - scale1;
      } else {
        extra = 0;
      }

      // num1 = (unsigned char *) safe_emalloc (1, n1.n_len+n1.n_scale, extra+2):
      num1 = Libbcmath.safe_emalloc(1, n1.n_len + n1.n_scale, extra + 2);
      if (num1 === null) {
        Libbcmath.bc_out_of_memory();
      }
      // memset (num1, 0, n1->n_len+n1->n_scale+extra+2):
      Libbcmath.memset(num1, 0, 0, n1.n_len + n1.n_scale + extra + 2);
      // memcpy (num1+1, n1.n_value, n1.n_len+n1.n_scale):
      Libbcmath.memcpy(num1, 1, n1.n_value, 0, n1.n_len + n1.n_scale);
      // len2 = n2->n_len + scale2:
      len2 = n2.n_len + scale2;
      // num2 = (unsigned char *) safe_emalloc (1, len2, 1):
      num2 = Libbcmath.safe_emalloc(1, len2, 1);
      if (num2 === null) {
        Libbcmath.bc_out_of_memory();
      }
      // memcpy (num2, n2.n_value, len2):
      Libbcmath.memcpy(num2, 0, n2.n_value, 0, len2);
      // *(num2+len2) = 0:
      num2[len2] = 0;
      // n2ptr = num2:
      n2ptr = 0;
      // while (*n2ptr === 0):
      while (num2[n2ptr] === 0) {
        n2ptr++;
        len2--;
      }

      // Calculate the number of quotient digits.
      if (len2 > len1 + scale) {
        qdigits = scale + 1;
        zero = true;
      } else {
        zero = false;
        if (len2 > len1) {
          qdigits = scale + 1; // One for the zero integer part.
        } else {
          qdigits = len1 - len2 + scale + 1;
        }
      }

      // Allocate and zero the storage for the quotient.
      // qval = bc_new_num (qdigits-scale,scale);
      qval = Libbcmath.bc_new_num(qdigits - scale, scale);
      // memset (qval->n_value, 0, qdigits);
      Libbcmath.memset(qval.n_value, 0, 0, qdigits);
      // Allocate storage for the temporary storage mval.
      // mval = (unsigned char *) safe_emalloc (1, len2, 1);
      mval = Libbcmath.safe_emalloc(1, len2, 1);
      if (mval === null) {
        Libbcmath.bc_out_of_memory();
      }

      // Now for the full divide algorithm.
      if (!zero) {
        // Normalize
        // norm = Libbcmath.cint(10 / (Libbcmath.cint(n2.n_value[n2ptr]) + 1));
        // norm =  10 / ((int)*n2ptr + 1)
        norm = Math.floor(10 / (n2.n_value[n2ptr] + 1)); // norm =  10 / ((int)*n2ptr + 1);
        if (norm !== 1) {
          // Libbcmath._one_mult(num1, len1+scale1+extra+1, norm, num1);
          Libbcmath._one_mult(num1, 0, len1 + scale1 + extra + 1, norm, num1, 0);
          // Libbcmath._one_mult(n2ptr, len2, norm, n2ptr);
          Libbcmath._one_mult(n2.n_value, n2ptr, len2, norm, n2.n_value, n2ptr);
          // @todo: Check: Is the pointer affected by the call? if so,
          // maybe need to adjust points on return?
        }

        // Initialize divide loop.
        qdig = 0;
        if (len2 > len1) {
          qptr = len2 - len1; // qptr = (unsigned char *) qval.n_value+len2-len1;
        } else {
          qptr = 0; // qptr = (unsigned char *) qval.n_value;
        }

        // Loop
        while (qdig <= len1 + scale - len2) {
          // Calculate the quotient digit guess.
          if (n2.n_value[n2ptr] === num1[qdig]) {
            qguess = 9;
          } else {
            qguess = Math.floor((num1[qdig] * 10 + num1[qdig + 1]) / n2.n_value[n2ptr]);
          }
          // Test qguess.

          if (n2.n_value[n2ptr + 1] * qguess > (num1[qdig] * 10 + num1[qdig + 1] - n2.n_value[n2ptr] * qguess) * 10 + num1[qdig + 2]) {
            qguess--;
            // And again.
            if (n2.n_value[n2ptr + 1] * qguess > (num1[qdig] * 10 + num1[qdig + 1] - n2.n_value[n2ptr] * qguess) * 10 + num1[qdig + 2]) {
              qguess--;
            }
          }

          // Multiply and subtract.
          borrow = 0;
          if (qguess !== 0) {
            mval[0] = 0; //* mval = 0; // @CHECK is this to fix ptr2 < 0?
            // _one_mult (n2ptr, len2, qguess, mval+1); // @CHECK
            Libbcmath._one_mult(n2.n_value, n2ptr, len2, qguess, mval, 1);
            ptr1 = qdig + len2; // (unsigned char *) num1+qdig+len2;
            ptr2 = len2; // (unsigned char *) mval+len2;
            // @todo: CHECK: Does a negative pointer return null?
            // ptr2 can be < 0 here as ptr1 = len2, thus count < len2+1 will always fail ?
            for (count = 0; count < len2 + 1; count++) {
              if (ptr2 < 0) {
                // val = Libbcmath.cint(num1[ptr1]) - 0 - borrow;
                // val = (int) *ptr1 - (int) *ptr2-- - borrow;
                val = num1[ptr1] - 0 - borrow; // val = (int) *ptr1 - (int) *ptr2-- - borrow;
              } else {
                // val = Libbcmath.cint(num1[ptr1]) - Libbcmath.cint(mval[ptr2--]) - borrow;
                // val = (int) *ptr1 - (int) *ptr2-- - borrow;
                // val = (int) *ptr1 - (int) *ptr2-- - borrow;
                val = num1[ptr1] - mval[ptr2--] - borrow;
              }
              if (val < 0) {
                val += 10;
                borrow = 1;
              } else {
                borrow = 0;
              }
              num1[ptr1--] = val;
            }
          }

          // Test for negative result.
          if (borrow === 1) {
            qguess--;
            ptr1 = qdig + len2; // (unsigned char *) num1+qdig+len2;
            ptr2 = len2 - 1; // (unsigned char *) n2ptr+len2-1;
            carry = 0;
            for (count = 0; count < len2; count++) {
              if (ptr2 < 0) {
                // val = Libbcmath.cint(num1[ptr1]) + 0 + carry;
                // val = (int) *ptr1 + (int) *ptr2-- + carry;
                // val = (int) *ptr1 + (int) *ptr2-- + carry;
                val = num1[ptr1] + 0 + carry;
              } else {
                // val = Libbcmath.cint(num1[ptr1]) + Libbcmath.cint(n2.n_value[ptr2--]) + carry;
                // val = (int) *ptr1 + (int) *ptr2-- + carry;
                // val = (int) *ptr1 + (int) *ptr2-- + carry;
                val = num1[ptr1] + n2.n_value[ptr2--] + carry;
              }
              if (val > 9) {
                val -= 10;
                carry = 1;
              } else {
                carry = 0;
              }
              num1[ptr1--] = val; //* ptr1-- = val;
            }
            if (carry === 1) {
              // num1[ptr1] = Libbcmath.cint((num1[ptr1] + 1) % 10);
              // *ptr1 = (*ptr1 + 1) % 10; // @CHECK
              // *ptr1 = (*ptr1 + 1) % 10; // @CHECK
              num1[ptr1] = (num1[ptr1] + 1) % 10;
            }
          }

          // We now know the quotient digit.
          qval.n_value[qptr++] = qguess; //* qptr++ =  qguess;
          qdig++;
        }
      }

      // Clean up and return the number.
      qval.n_sign = n1.n_sign === n2.n_sign ? Libbcmath.PLUS : Libbcmath.MINUS;
      if (Libbcmath.bc_is_zero(qval)) {
        qval.n_sign = Libbcmath.PLUS;
      }
      Libbcmath._bc_rm_leading_zeros(qval);

      return qval;

      // return 0;    // Everything is OK.
    },

    MUL_BASE_DIGITS: 80,
    MUL_SMALL_DIGITS: 80 / 4,
    // #define MUL_SMALL_DIGITS mul_base_digits/4

    /* The multiply routine.  N2 times N1 is put int PROD with the scale of
    the result being MIN(N2 scale+N1 scale, MAX (SCALE, N2 scale, N1 scale)).
    */
    /**
     * @param n1 bc_num
     * @param n2 bc_num
     * @param scale [int] optional
     */
    bc_multiply: function bc_multiply(n1, n2, scale) {
      var pval; // bc_num
      var len1, len2; // int
      var fullScale, prodScale; // int
      // Initialize things.
      len1 = n1.n_len + n1.n_scale;
      len2 = n2.n_len + n2.n_scale;
      fullScale = n1.n_scale + n2.n_scale;
      prodScale = Libbcmath.MIN(fullScale, Libbcmath.MAX(scale, Libbcmath.MAX(n1.n_scale, n2.n_scale)));

      // pval = Libbcmath.bc_init_num(); // allow pass by ref
      // Do the multiply
      pval = Libbcmath._bc_rec_mul(n1, len1, n2, len2, fullScale);

      // Assign to prod and clean up the number.
      pval.n_sign = n1.n_sign === n2.n_sign ? Libbcmath.PLUS : Libbcmath.MINUS;
      // pval.n_value = pval.nPtr;
      pval.n_len = len2 + len1 + 1 - fullScale;
      pval.n_scale = prodScale;
      Libbcmath._bc_rm_leading_zeros(pval);
      if (Libbcmath.bc_is_zero(pval)) {
        pval.n_sign = Libbcmath.PLUS;
      }
      // bc_free_num (prod);
      return pval;
    },

    new_sub_num: function new_sub_num(length, scale, value) {
      var ptr = arguments.length > 3 && arguments[3] !== undefined ? arguments[3] : 0;

      var temp = new Libbcmath.bc_num(); // eslint-disable-line new-cap
      temp.n_sign = Libbcmath.PLUS;
      temp.n_len = length;
      temp.n_scale = scale;
      temp.n_value = Libbcmath.safe_emalloc(1, length + scale, 0);
      Libbcmath.memcpy(temp.n_value, 0, value, ptr, length + scale);
      return temp;
    },

    _bc_simp_mul: function _bc_simp_mul(n1, n1len, n2, n2len, fullScale) {
      var prod; // bc_num
      var n1ptr, n2ptr, pvptr; // char *n1ptr, *n2ptr, *pvptr;
      var n1end, n2end; // char *n1end, *n2end;        // To the end of n1 and n2.
      var indx, sum, prodlen; // int indx, sum, prodlen;
      prodlen = n1len + n2len + 1;

      prod = Libbcmath.bc_new_num(prodlen, 0);

      n1end = n1len - 1; // (char *) (n1->n_value + n1len - 1);
      n2end = n2len - 1; // (char *) (n2->n_value + n2len - 1);
      pvptr = prodlen - 1; // (char *) ((*prod)->n_value + prodlen - 1);
      sum = 0;

      // Here is the loop...
      for (indx = 0; indx < prodlen - 1; indx++) {
        // (char *) (n1end - MAX(0, indx-n2len+1));
        n1ptr = n1end - Libbcmath.MAX(0, indx - n2len + 1);
        // (char *) (n2end - MIN(indx, n2len-1));
        n2ptr = n2end - Libbcmath.MIN(indx, n2len - 1);
        while (n1ptr >= 0 && n2ptr <= n2end) {
          // sum += *n1ptr-- * *n2ptr++;
          sum += n1.n_value[n1ptr--] * n2.n_value[n2ptr++];
        }
        //* pvptr-- = sum % BASE;
        prod.n_value[pvptr--] = Math.floor(sum % Libbcmath.BASE);
        sum = Math.floor(sum / Libbcmath.BASE); // sum = sum / BASE;
      }
      prod.n_value[pvptr] = sum; //* pvptr = sum;
      return prod;
    },

    /* A special adder/subtractor for the recursive divide and conquer
       multiply algorithm.  Note: if sub is called, accum must
       be larger that what is being subtracted.  Also, accum and val
       must have n_scale = 0.  (e.g. they must look like integers. *) */
    _bc_shift_addsub: function _bc_shift_addsub(accum, val, shift, sub) {
      var accp, valp; // signed char *accp, *valp;
      var count, carry; // int  count, carry;
      count = val.n_len;
      if (val.n_value[0] === 0) {
        count--;
      }

      // assert (accum->n_len+accum->n_scale >= shift+count);
      if (accum.n_len + accum.n_scale < shift + count) {
        throw new Error('len + scale < shift + count'); // ?? I think that's what assert does :)
      }

      // Set up pointers and others
      // (signed char *)(accum->n_value + accum->n_len + accum->n_scale - shift - 1);
      accp = accum.n_len + accum.n_scale - shift - 1;
      valp = val.n_len - 1; // (signed char *)(val->n_value + val->n_len - 1);
      carry = 0;
      if (sub) {
        // Subtraction, carry is really borrow.
        while (count--) {
          accum.n_value[accp] -= val.n_value[valp--] + carry; //* accp -= *valp-- + carry;
          if (accum.n_value[accp] < 0) {
            // if (*accp < 0)
            carry = 1;
            accum.n_value[accp--] += Libbcmath.BASE; //* accp-- += BASE;
          } else {
            carry = 0;
            accp--;
          }
        }
        while (carry) {
          accum.n_value[accp] -= carry; //* accp -= carry;
          if (accum.n_value[accp] < 0) {
            // if (*accp < 0)
            accum.n_value[accp--] += Libbcmath.BASE; //    *accp-- += BASE;
          } else {
            carry = 0;
          }
        }
      } else {
        // Addition
        while (count--) {
          accum.n_value[accp] += val.n_value[valp--] + carry; //* accp += *valp-- + carry;
          if (accum.n_value[accp] > Libbcmath.BASE - 1) {
            // if (*accp > (BASE-1))
            carry = 1;
            accum.n_value[accp--] -= Libbcmath.BASE; //* accp-- -= BASE;
          } else {
            carry = 0;
            accp--;
          }
        }
        while (carry) {
          accum.n_value[accp] += carry; //* accp += carry;
          if (accum.n_value[accp] > Libbcmath.BASE - 1) {
            // if (*accp > (BASE-1))
            accum.n_value[accp--] -= Libbcmath.BASE; //* accp-- -= BASE;
          } else {
            carry = 0;
          }
        }
      }
      return true; // accum is the pass-by-reference return
    },

    /* Recursive divide and conquer multiply algorithm.
       based on
       Let u = u0 + u1*(b^n)
       Let v = v0 + v1*(b^n)
       Then uv = (B^2n+B^n)*u1*v1 + B^n*(u1-u0)*(v0-v1) + (B^n+1)*u0*v0
        B is the base of storage, number of digits in u1,u0 close to equal.
    */
    _bc_rec_mul: function _bc_rec_mul(u, ulen, v, vlen, fullScale) {
      var prod; // @return
      var u0, u1, v0, v1; // bc_num
      // var u0len,
      // var v0len // int
      var m1, m2, m3, d1, d2; // bc_num
      var n, prodlen, m1zero; // int
      var d1len, d2len; // int
      // Base case?
      if (ulen + vlen < Libbcmath.MUL_BASE_DIGITS || ulen < Libbcmath.MUL_SMALL_DIGITS || vlen < Libbcmath.MUL_SMALL_DIGITS) {
        return Libbcmath._bc_simp_mul(u, ulen, v, vlen, fullScale);
      }

      // Calculate n -- the u and v split point in digits.
      n = Math.floor((Libbcmath.MAX(ulen, vlen) + 1) / 2);

      // Split u and v.
      if (ulen < n) {
        u1 = Libbcmath.bc_init_num(); // u1 = bc_copy_num (BCG(_zero_));
        u0 = Libbcmath.new_sub_num(ulen, 0, u.n_value);
      } else {
        u1 = Libbcmath.new_sub_num(ulen - n, 0, u.n_value);
        u0 = Libbcmath.new_sub_num(n, 0, u.n_value, ulen - n);
      }
      if (vlen < n) {
        v1 = Libbcmath.bc_init_num(); // bc_copy_num (BCG(_zero_));
        v0 = Libbcmath.new_sub_num(vlen, 0, v.n_value);
      } else {
        v1 = Libbcmath.new_sub_num(vlen - n, 0, v.n_value);
        v0 = Libbcmath.new_sub_num(n, 0, v.n_value, vlen - n);
      }
      Libbcmath._bc_rm_leading_zeros(u1);
      Libbcmath._bc_rm_leading_zeros(u0);
      // var u0len = u0.n_len
      Libbcmath._bc_rm_leading_zeros(v1);
      Libbcmath._bc_rm_leading_zeros(v0);
      // var v0len = v0.n_len

      m1zero = Libbcmath.bc_is_zero(u1) || Libbcmath.bc_is_zero(v1);

      // Calculate sub results ...
      d1 = Libbcmath.bc_init_num(); // needed?
      d2 = Libbcmath.bc_init_num(); // needed?
      d1 = Libbcmath.bc_sub(u1, u0, 0);
      d1len = d1.n_len;

      d2 = Libbcmath.bc_sub(v0, v1, 0);
      d2len = d2.n_len;

      // Do recursive multiplies and shifted adds.
      if (m1zero) {
        m1 = Libbcmath.bc_init_num(); // bc_copy_num (BCG(_zero_));
      } else {
        // m1 = Libbcmath.bc_init_num(); //allow pass-by-ref
        m1 = Libbcmath._bc_rec_mul(u1, u1.n_len, v1, v1.n_len, 0);
      }
      if (Libbcmath.bc_is_zero(d1) || Libbcmath.bc_is_zero(d2)) {
        m2 = Libbcmath.bc_init_num(); // bc_copy_num (BCG(_zero_));
      } else {
        // m2 = Libbcmath.bc_init_num(); //allow pass-by-ref
        m2 = Libbcmath._bc_rec_mul(d1, d1len, d2, d2len, 0);
      }

      if (Libbcmath.bc_is_zero(u0) || Libbcmath.bc_is_zero(v0)) {
        m3 = Libbcmath.bc_init_num(); // bc_copy_num (BCG(_zero_));
      } else {
        // m3 = Libbcmath.bc_init_num(); //allow pass-by-ref
        m3 = Libbcmath._bc_rec_mul(u0, u0.n_len, v0, v0.n_len, 0);
      }

      // Initialize product
      prodlen = ulen + vlen + 1;
      prod = Libbcmath.bc_new_num(prodlen, 0);

      if (!m1zero) {
        Libbcmath._bc_shift_addsub(prod, m1, 2 * n, 0);
        Libbcmath._bc_shift_addsub(prod, m1, n, 0);
      }
      Libbcmath._bc_shift_addsub(prod, m3, n, 0);
      Libbcmath._bc_shift_addsub(prod, m3, 0, 0);
      Libbcmath._bc_shift_addsub(prod, m2, n, d1.n_sign !== d2.n_sign);

      return prod;
      // Now clean up!
      // bc_free_num (&u1);
      // bc_free_num (&u0);
      // bc_free_num (&v1);
      // bc_free_num (&m1);
      // bc_free_num (&v0);
      // bc_free_num (&m2);
      // bc_free_num (&m3);
      // bc_free_num (&d1);
      // bc_free_num (&d2);
    },

    /**
     *
     * @param {bc_num} n1
     * @param {bc_num} n2
     * @param {boolean} useSign
     * @param {boolean} ignoreLast
     * @return -1, 0, 1 (see bc_compare)
     */
    _bc_do_compare: function _bc_do_compare(n1, n2, useSign, ignoreLast) {
      var n1ptr, n2ptr; // int
      var count; // int
      // First, compare signs.
      if (useSign && n1.n_sign !== n2.n_sign) {
        if (n1.n_sign === Libbcmath.PLUS) {
          return 1; // Positive N1 > Negative N2
        } else {
          return -1; // Negative N1 < Positive N1
        }
      }

      // Now compare the magnitude.
      if (n1.n_len !== n2.n_len) {
        if (n1.n_len > n2.n_len) {
          // Magnitude of n1 > n2.
          if (!useSign || n1.n_sign === Libbcmath.PLUS) {
            return 1;
          } else {
            return -1;
          }
        } else {
          // Magnitude of n1 < n2.
          if (!useSign || n1.n_sign === Libbcmath.PLUS) {
            return -1;
          } else {
            return 1;
          }
        }
      }

      /* If we get here, they have the same number of integer digits.
      check the integer part and the equal length part of the fraction. */
      count = n1.n_len + Math.min(n1.n_scale, n2.n_scale);
      n1ptr = 0;
      n2ptr = 0;

      while (count > 0 && n1.n_value[n1ptr] === n2.n_value[n2ptr]) {
        n1ptr++;
        n2ptr++;
        count--;
      }

      if (ignoreLast && count === 1 && n1.n_scale === n2.n_scale) {
        return 0;
      }

      if (count !== 0) {
        if (n1.n_value[n1ptr] > n2.n_value[n2ptr]) {
          // Magnitude of n1 > n2.
          if (!useSign || n1.n_sign === Libbcmath.PLUS) {
            return 1;
          } else {
            return -1;
          }
        } else {
          // Magnitude of n1 < n2.
          if (!useSign || n1.n_sign === Libbcmath.PLUS) {
            return -1;
          } else {
            return 1;
          }
        }
      }

      // They are equal up to the last part of the equal part of the fraction.
      if (n1.n_scale !== n2.n_scale) {
        if (n1.n_scale > n2.n_scale) {
          for (count = n1.n_scale - n2.n_scale; count > 0; count--) {
            if (n1.n_value[n1ptr++] !== 0) {
              // Magnitude of n1 > n2.
              if (!useSign || n1.n_sign === Libbcmath.PLUS) {
                return 1;
              } else {
                return -1;
              }
            }
          }
        } else {
          for (count = n2.n_scale - n1.n_scale; count > 0; count--) {
            if (n2.n_value[n2ptr++] !== 0) {
              // Magnitude of n1 < n2.
              if (!useSign || n1.n_sign === Libbcmath.PLUS) {
                return -1;
              } else {
                return 1;
              }
            }
          }
        }
      }

      // They must be equal!
      return 0;
    },

    /* Here is the full subtract routine that takes care of negative numbers.
    N2 is subtracted from N1 and the result placed in RESULT.  SCALE_MIN
    is the minimum scale for the result. */
    bc_sub: function bc_sub(n1, n2, scaleMin) {
      var diff; // bc_num
      var cmpRes, resScale; // int
      if (n1.n_sign !== n2.n_sign) {
        diff = Libbcmath._bc_do_add(n1, n2, scaleMin);
        diff.n_sign = n1.n_sign;
      } else {
        // subtraction must be done.
        // Compare magnitudes.
        cmpRes = Libbcmath._bc_do_compare(n1, n2, false, false);
        switch (cmpRes) {
          case -1:
            // n1 is less than n2, subtract n1 from n2.
            diff = Libbcmath._bc_do_sub(n2, n1, scaleMin);
            diff.n_sign = n2.n_sign === Libbcmath.PLUS ? Libbcmath.MINUS : Libbcmath.PLUS;
            break;
          case 0:
            // They are equal! return zero!
            resScale = Libbcmath.MAX(scaleMin, Libbcmath.MAX(n1.n_scale, n2.n_scale));
            diff = Libbcmath.bc_new_num(1, resScale);
            Libbcmath.memset(diff.n_value, 0, 0, resScale + 1);
            break;
          case 1:
            // n2 is less than n1, subtract n2 from n1.
            diff = Libbcmath._bc_do_sub(n1, n2, scaleMin);
            diff.n_sign = n1.n_sign;
            break;
        }
      }

      // Clean up and return.
      // bc_free_num (result);
      //* result = diff;
      return diff;
    },

    _bc_do_add: function _bc_do_add(n1, n2, scaleMin) {
      var sum; // bc_num
      var sumScale, sumDigits; // int
      var n1ptr, n2ptr, sumptr; // int
      var carry, n1bytes, n2bytes; // int
      var tmp; // int

      // Prepare sum.
      sumScale = Libbcmath.MAX(n1.n_scale, n2.n_scale);
      sumDigits = Libbcmath.MAX(n1.n_len, n2.n_len) + 1;
      sum = Libbcmath.bc_new_num(sumDigits, Libbcmath.MAX(sumScale, scaleMin));

      // Start with the fraction part.  Initialize the pointers.
      n1bytes = n1.n_scale;
      n2bytes = n2.n_scale;
      n1ptr = n1.n_len + n1bytes - 1;
      n2ptr = n2.n_len + n2bytes - 1;
      sumptr = sumScale + sumDigits - 1;

      // Add the fraction part.  First copy the longer fraction
      // (ie when adding 1.2345 to 1 we know .2345 is correct already) .
      if (n1bytes !== n2bytes) {
        if (n1bytes > n2bytes) {
          // n1 has more dp then n2
          while (n1bytes > n2bytes) {
            sum.n_value[sumptr--] = n1.n_value[n1ptr--];
            // *sumptr-- = *n1ptr--;
            n1bytes--;
          }
        } else {
          // n2 has more dp then n1
          while (n2bytes > n1bytes) {
            sum.n_value[sumptr--] = n2.n_value[n2ptr--];
            // *sumptr-- = *n2ptr--;
            n2bytes--;
          }
        }
      }

      // Now add the remaining fraction part and equal size integer parts.
      n1bytes += n1.n_len;
      n2bytes += n2.n_len;
      carry = 0;
      while (n1bytes > 0 && n2bytes > 0) {
        // add the two numbers together
        tmp = n1.n_value[n1ptr--] + n2.n_value[n2ptr--] + carry;
        // *sumptr = *n1ptr-- + *n2ptr-- + carry;
        // check if they are >= 10 (impossible to be more then 18)
        if (tmp >= Libbcmath.BASE) {
          carry = 1;
          tmp -= Libbcmath.BASE; // yep, subtract 10, add a carry
        } else {
          carry = 0;
        }
        sum.n_value[sumptr] = tmp;
        sumptr--;
        n1bytes--;
        n2bytes--;
      }

      // Now add carry the [rest of the] longer integer part.
      if (n1bytes === 0) {
        // n2 is a bigger number then n1
        while (n2bytes-- > 0) {
          tmp = n2.n_value[n2ptr--] + carry;
          // *sumptr = *n2ptr-- + carry;
          if (tmp >= Libbcmath.BASE) {
            carry = 1;
            tmp -= Libbcmath.BASE;
          } else {
            carry = 0;
          }
          sum.n_value[sumptr--] = tmp;
        }
      } else {
        // n1 is bigger then n2..
        while (n1bytes-- > 0) {
          tmp = n1.n_value[n1ptr--] + carry;
          // *sumptr = *n1ptr-- + carry;
          if (tmp >= Libbcmath.BASE) {
            carry = 1;
            tmp -= Libbcmath.BASE;
          } else {
            carry = 0;
          }
          sum.n_value[sumptr--] = tmp;
        }
      }

      // Set final carry.
      if (carry === 1) {
        sum.n_value[sumptr] += 1;
        // *sumptr += 1;
      }

      // Adjust sum and return.
      Libbcmath._bc_rm_leading_zeros(sum);
      return sum;
    },

    /**
     * Perform a subtraction
     *
     * Perform subtraction: N2 is subtracted from N1 and the value is
     *  returned.  The signs of N1 and N2 are ignored.  Also, N1 is
     *  assumed to be larger than N2.  SCALE_MIN is the minimum scale
     *  of the result.
     *
     * Basic school maths says to subtract 2 numbers..
     * 1. make them the same length, the decimal places, and the integer part
     * 2. start from the right and subtract the two numbers from each other
     * 3. if the sum of the 2 numbers < 0, carry -1 to the next set and add 10
     * (ie 18 > carry 1 becomes 8). thus 0.9 + 0.9 = 1.8
     *
     * @param {bc_num} n1
     * @param {bc_num} n2
     * @param {int} scaleMin
     * @return bc_num
     */
    _bc_do_sub: function _bc_do_sub(n1, n2, scaleMin) {
      var diff; // bc_num
      var diffScale, diffLen; // int
      var minScale, minLen; // int
      var n1ptr, n2ptr, diffptr; // int
      var borrow, count, val; // int
      // Allocate temporary storage.
      diffLen = Libbcmath.MAX(n1.n_len, n2.n_len);
      diffScale = Libbcmath.MAX(n1.n_scale, n2.n_scale);
      minLen = Libbcmath.MIN(n1.n_len, n2.n_len);
      minScale = Libbcmath.MIN(n1.n_scale, n2.n_scale);
      diff = Libbcmath.bc_new_num(diffLen, Libbcmath.MAX(diffScale, scaleMin));

      /* Not needed?
      // Zero extra digits made by scaleMin.
      if (scaleMin > diffScale) {
        diffptr = (char *) (diff->n_value + diffLen + diffScale);
        for (count = scaleMin - diffScale; count > 0; count--) {
          *diffptr++ = 0;
        }
      }
      */

      // Initialize the subtract.
      n1ptr = n1.n_len + n1.n_scale - 1;
      n2ptr = n2.n_len + n2.n_scale - 1;
      diffptr = diffLen + diffScale - 1;

      // Subtract the numbers.
      borrow = 0;

      // Take care of the longer scaled number.
      if (n1.n_scale !== minScale) {
        // n1 has the longer scale
        for (count = n1.n_scale - minScale; count > 0; count--) {
          diff.n_value[diffptr--] = n1.n_value[n1ptr--];
          // *diffptr-- = *n1ptr--;
        }
      } else {
        // n2 has the longer scale
        for (count = n2.n_scale - minScale; count > 0; count--) {
          val = 0 - n2.n_value[n2ptr--] - borrow;
          // val = - *n2ptr-- - borrow;
          if (val < 0) {
            val += Libbcmath.BASE;
            borrow = 1;
          } else {
            borrow = 0;
          }
          diff.n_value[diffptr--] = val;
          //* diffptr-- = val;
        }
      }

      // Now do the equal length scale and integer parts.
      for (count = 0; count < minLen + minScale; count++) {
        val = n1.n_value[n1ptr--] - n2.n_value[n2ptr--] - borrow;
        // val = *n1ptr-- - *n2ptr-- - borrow;
        if (val < 0) {
          val += Libbcmath.BASE;
          borrow = 1;
        } else {
          borrow = 0;
        }
        diff.n_value[diffptr--] = val;
        //* diffptr-- = val;
      }

      // If n1 has more digits then n2, we now do that subtract.
      if (diffLen !== minLen) {
        for (count = diffLen - minLen; count > 0; count--) {
          val = n1.n_value[n1ptr--] - borrow;
          // val = *n1ptr-- - borrow;
          if (val < 0) {
            val += Libbcmath.BASE;
            borrow = 1;
          } else {
            borrow = 0;
          }
          diff.n_value[diffptr--] = val;
        }
      }

      // Clean up and return.
      Libbcmath._bc_rm_leading_zeros(diff);
      return diff;
    },

    /**
     *
     * @param {int} length
     * @param {int} scale
     * @return bc_num
     */
    bc_new_num: function bc_new_num(length, scale) {
      var temp; // bc_num
      temp = new Libbcmath.bc_num(); // eslint-disable-line new-cap
      temp.n_sign = Libbcmath.PLUS;
      temp.n_len = length;
      temp.n_scale = scale;
      temp.n_value = Libbcmath.safe_emalloc(1, length + scale, 0);
      Libbcmath.memset(temp.n_value, 0, 0, length + scale);
      return temp;
    },

    safe_emalloc: function safe_emalloc(size, len, extra) {
      return Array(size * len + extra);
    },

    /**
     * Create a new number
     */
    bc_init_num: function bc_init_num() {
      return new Libbcmath.bc_new_num(1, 0); // eslint-disable-line new-cap
    },

    _bc_rm_leading_zeros: function _bc_rm_leading_zeros(num) {
      // We can move n_value to point to the first non zero digit!
      while (num.n_value[0] === 0 && num.n_len > 1) {
        num.n_value.shift();
        num.n_len--;
      }
    },

    /**
     * Convert to bc_num detecting scale
     */
    php_str2num: function php_str2num(str) {
      var p;
      p = str.indexOf('.');
      if (p === -1) {
        return Libbcmath.bc_str2num(str, 0);
      } else {
        return Libbcmath.bc_str2num(str, str.length - p);
      }
    },

    CH_VAL: function CH_VAL(c) {
      return c - '0'; // ??
    },

    BCD_CHAR: function BCD_CHAR(d) {
      return d + '0'; // ??
    },

    isdigit: function isdigit(c) {
      return isNaN(parseInt(c, 10));
    },

    bc_str2num: function bc_str2num(strIn, scale) {
      var str, num, ptr, digits, strscale, zeroInt, nptr;
      // remove any non-expected characters
      // Check for valid number and count digits.

      str = strIn.split(''); // convert to array
      ptr = 0; // str
      digits = 0;
      strscale = 0;
      zeroInt = false;
      if (str[ptr] === '+' || str[ptr] === '-') {
        ptr++; // Sign
      }
      while (str[ptr] === '0') {
        ptr++; // Skip leading zeros.
      }
      // while (Libbcmath.isdigit(str[ptr])) {
      while (str[ptr] % 1 === 0) {
        // Libbcmath.isdigit(str[ptr])) {
        ptr++;
        digits++; // digits
      }

      if (str[ptr] === '.') {
        ptr++; // decimal point
      }
      // while (Libbcmath.isdigit(str[ptr])) {
      while (str[ptr] % 1 === 0) {
        // Libbcmath.isdigit(str[ptr])) {
        ptr++;
        strscale++; // digits
      }

      if (str[ptr] || digits + strscale === 0) {
        // invalid number, return 0
        return Libbcmath.bc_init_num();
        //* num = bc_copy_num (BCG(_zero_));
      }

      // Adjust numbers and allocate storage and initialize fields.
      strscale = Libbcmath.MIN(strscale, scale);
      if (digits === 0) {
        zeroInt = true;
        digits = 1;
      }

      num = Libbcmath.bc_new_num(digits, strscale);

      // Build the whole number.
      ptr = 0; // str
      if (str[ptr] === '-') {
        num.n_sign = Libbcmath.MINUS;
        // (*num)->n_sign = MINUS;
        ptr++;
      } else {
        num.n_sign = Libbcmath.PLUS;
        // (*num)->n_sign = PLUS;
        if (str[ptr] === '+') {
          ptr++;
        }
      }
      while (str[ptr] === '0') {
        ptr++; // Skip leading zeros.
      }

      nptr = 0; // (*num)->n_value;
      if (zeroInt) {
        num.n_value[nptr++] = 0;
        digits = 0;
      }
      for (; digits > 0; digits--) {
        num.n_value[nptr++] = Libbcmath.CH_VAL(str[ptr++]);
        //* nptr++ = CH_VAL(*ptr++);
      }

      // Build the fractional part.
      if (strscale > 0) {
        ptr++; // skip the decimal point!
        for (; strscale > 0; strscale--) {
          num.n_value[nptr++] = Libbcmath.CH_VAL(str[ptr++]);
        }
      }

      return num;
    },

    cint: function cint(v) {
      if (typeof v === 'undefined') {
        v = 0;
      }
      var x = parseInt(v, 10);
      if (isNaN(x)) {
        x = 0;
      }
      return x;
    },

    /**
     * Basic min function
     * @param {int} a
     * @param {int} b
     */
    MIN: function MIN(a, b) {
      return a > b ? b : a;
    },

    /**
     * Basic max function
     * @param {int} a
     * @param {int} b
     */
    MAX: function MAX(a, b) {
      return a > b ? a : b;
    },

    /**
     * Basic odd function
     * @param {int} a
     */
    ODD: function ODD(a) {
      return a & 1;
    },

    /**
     * replicate c function
     * @param {array} r     return (by reference)
     * @param {int} ptr
     * @param {string} chr    char to fill
     * @param {int} len       length to fill
     */
    memset: function memset(r, ptr, chr, len) {
      var i;
      for (i = 0; i < len; i++) {
        r[ptr + i] = chr;
      }
    },

    /**
     * Replacement c function
     * Obviously can't work like c does, so we've added an "offset"
     * param so you could do memcpy(dest+1, src, len) as memcpy(dest, 1, src, len)
     * Also only works on arrays
     */
    memcpy: function memcpy(dest, ptr, src, srcptr, len) {
      var i;
      for (i = 0; i < len; i++) {
        dest[ptr + i] = src[srcptr + i];
      }
      return true;
    },

    /**
     * Determine if the number specified is zero or not
     * @param {bc_num} num    number to check
     * @return boolean      true when zero, false when not zero.
     */
    bc_is_zero: function bc_is_zero(num) {
      var count; // int
      var nptr; // int
      // Quick check.
      // if (num === BCG(_zero_)) return TRUE;
      // Initialize
      count = num.n_len + num.n_scale;
      nptr = 0; // num->n_value;
      // The check
      while (count > 0 && num.n_value[nptr++] === 0) {
        count--;
      }

      if (count !== 0) {
        return false;
      } else {
        return true;
      }
    },

    bc_out_of_memory: function bc_out_of_memory() {
      throw new Error('(BC) Out of memory');
    }
  };
  return Libbcmath;
};
//# sourceMappingURL=_bc.js.map