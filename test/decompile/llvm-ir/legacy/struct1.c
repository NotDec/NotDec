struct STDEMO{
  int val_1;
  int val_2;
  int val_3;
  };

  struct STDEMO* func_demo(struct STDEMO* d)
  {d->val_1+=1;d->val_2+=2;return d;}

  void test_demo() {
    struct STDEMO st;
    st.val_1 = 1;
    st.val_2 = 2;
    st.val_3 = 3;
    struct STDEMO* dptr = func_demo(&st);
    dptr->val_3 += 3;
  }
