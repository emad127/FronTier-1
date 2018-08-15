#include "gmock/gmock.h"
#include "../BV.h"


class AABBTestData : public ::testing::Test
{
    public:

    TRI *t1, *t2, *t3;
    BOND *s1, *s2;
    POINT *a, *b, *c, *d, *e, *f, *g;

    FT_TRI *T1, *T2, *T3;
    FT_BOND *B1, *B2;
    FT_POINT* P;

    BV_Point L = {-1,-1,-1};
    BV_Point U = {1,1,1};

    //can probably make this a static StartUp/TearDown method
    //in the derived class fixture to share the resources.
    AABBTestData()
    {
        a = new POINT;         b = new POINT;
        Coords(a)[0] = 2.0;    Coords(b)[0] = 1.0;
        Coords(a)[1] = 2.0;    Coords(b)[1] = 1.0;
        Coords(a)[2] = 3.0;    Coords(b)[2] = 1.0;

        c = new POINT;          d = new POINT;
        Coords(c)[0] = 9.0;     Coords(d)[0] = 10.0;
        Coords(c)[1] = 0.5;     Coords(d)[1] = 0.0;
        Coords(c)[2] = 2.0;     Coords(d)[2] = 0.0;

        e = new POINT;          f = new POINT;
        Coords(e)[0] = 0.0;     Coords(f)[0] = 0.0;
        Coords(e)[1] = 10.0;    Coords(f)[1] = 0.0;
        Coords(e)[2] = 0.0;     Coords(f)[2] = 10.0;

        g = new POINT;     //    h = new POINT;
        Coords(g)[0] = 0.0;//    Coords(h)[0] = 0.0;
        Coords(g)[1] = 0.0;//    Coords(h)[1] = 0.0;
        Coords(g)[2] = 0.0;//    Coords(h)[2] = 10.0;

        t1 = new TRI;                t2 = new TRI;
        Point_of_tri(t1)[0] = a;     Point_of_tri(t2)[0] = d;
        Point_of_tri(t1)[1] = b;     Point_of_tri(t2)[1] = e;
        Point_of_tri(t1)[2] = c;     Point_of_tri(t2)[2] = f;

        t3 = new TRI;           //     t2 = new TRI;
        Point_of_tri(t3)[0] = e;//     Point_of_tri(t2)[0] = d;
        Point_of_tri(t3)[1] = f;//     Point_of_tri(t2)[1] = e;
        Point_of_tri(t3)[2] = g;//     Point_of_tri(t2)[2] = f;

        s1 = new BOND;      s2 = new BOND;
        s1->start = a;      s2->start = c;
        s1->end = b;        s2->end = f;

        T1 = new FT_TRI(t1);    T2 = new FT_TRI(t2);
        T3 = new FT_TRI(t3);

        B1 = new FT_BOND(s1);   B2 = new FT_BOND(s2);
        P = new FT_POINT(a);
    }

    virtual void TearDown()
    {
        delete a; delete b; delete c;
        delete d; delete e; delete f;
        delete g; delete s1; delete s2;
        delete t1; delete t2; delete t3;
        delete P; delete B1; delete B2;
        delete T1; delete T2; delete T3;
    }

    virtual ~AABBTestData() = default;

};

/*
class AABBTests : public AABBTestData
{
    public:
    
    AABB *bbT1, *bbT2, *bbT3, *bbB1,
         *bbB2, *bbBVPts;

    AABBTests()
        : AABBTestData{}
    {
        bbT1 = new AABB(T1);
        bbT2 = new AABB(T2);
        bbT3 = new AABB(T3);
        bbB1 = new AABB(B1);
        bbB2 = new AABB(B2);

        BV_Point L = {-1,-1,-1};
        BV_Point U = {1,1,1};
        bbBVPts = new AABB(L,U);
    }

    void TearDown() override
    {
        delete bbB1; delete bbB2;
        delete bbT1; delete bbT2;
        delete bbT3; delete bbBVPts;
        AABBTestData::TearDown();
    }

    ~AABBTests() = default;

};
*/

class AABBTests : public AABBTestData
{
    public:
    
    AABB bbT1, bbT2, bbT3, bbB1,
         bbB2, bbBVPts;

    AABBTests()
        : AABBTestData{},
        bbT1{T1}, bbT2{T2}, bbT3{T3},
        bbB1{B1}, bbB2{B2}, bbBVPts{L,U}
    {}

    void TearDown() override
    {
        AABBTestData::TearDown();
    }

    ~AABBTests() = default;

};


using DISABLED_AABBTests = AABBTests;

TEST_F(DISABLED_AABBTests, mergeBoxesDeathTest)
{
    //AABB* bbNull;
    //ASSERT_DEATH(AABB::merge(bbT1,bbNull),"");
}

TEST_F(DISABLED_AABBTests, MergeBoxes)
{
    //AABB* parentbox = AABB::merge(bbT2,bbT3);
    //ASSERT_NE(parentbox,nullptr);
    //TODO: Add tests for properties of the new box.
    //      (see BoxesOverlapvsContain test)
}

TEST_F(AABBTests, BoxesOverlapVsContain)
{
    //contains means strictly contained
    EXPECT_TRUE(bbT2.contains(bbT1));
    EXPECT_FALSE(bbT1.contains(bbB1));

    //shared surface is considered overlap
    EXPECT_TRUE(bbT1.overlaps(bbB1));
    EXPECT_TRUE(bbT1.overlaps(bbB2));
    //TODO: Is this the behavior we really want?
    //      May need to add case(s) of
    //      loose containment (share surface)
    //      and/or not contained but share surfaces
}

TEST_F(AABBTests, ConstructorAABBs)
{
    AABB parentbox(bbT2,bbT3);
}

TEST_F(AABBTests, ConstructorBV_Points)
{
    ASSERT_DOUBLE_EQ(bbBVPts.centroid[0],0.0);
    ASSERT_DOUBLE_EQ(bbBVPts.centroid[1],0.0);
    ASSERT_DOUBLE_EQ(bbBVPts.centroid[2],0.0);
}

TEST_F(AABBTests, ConstructorFT_HSE)
{
    //See AABBTestData fixture
    ASSERT_DOUBLE_EQ(bbT1.upper[0],9.0);
    ASSERT_DOUBLE_EQ(bbT1.centroid[1],1.25);
    ASSERT_DOUBLE_EQ(bbT1.lower[2],1.0);
}

//TODO: Degenerate Cases?:
//      
//      1.  Box has zero volume for a point;
//          should AABB be allowed to be given
//          a point as a constructor argument?
//      
//      2.  What about for bonds or tris that are parallel
//          to two axis simultaneously and box is 2d?
//          (resulting in volume = 0)
//      
//      3.  Is volume even necessary, since using
//          hilbert curve for BVH construction?
//          




