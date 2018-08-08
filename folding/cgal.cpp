#include "cgal.h"
#include <stdexcept>
#include <utility>

cgalSurf::cgalSurf(INTERFACE* intfc, SURFACE** surf) :  
		c_intfc(intfc), c_surf(surf) 
{ 
    k_dx = 0.5; 
    c_bound = 0.125; 
    num_finite_face = 0; 
}

void cgalSurf::getExtraConstPoint(std::ifstream& fin)
{
    int numCons = 0;

    if (findAndLocate(fin, "Enter number of extra constraints:"))
        fin >> numCons;
    std::cout << numCons << std::endl;
    for (int i = 0; i < numCons; i++)
    {
         if (findAndLocate(fin, "Enter extra constraint points pair:"))
         {
             double p1[2], p2[2];

             fin >> p1[0] >> p1[1];
             fin >> p2[0] >> p2[1];
             std::cout << p1[0] << " " << p1[1] << std::endl;
             std::cout << p2[0] << " " << p2[1] << std::endl;
	     extConPoint.push_back(p1[0]);
	     extConPoint.push_back(p1[1]);
	     extConPoint.push_back(p2[0]);
             extConPoint.push_back(p2[1]);
         }
    }
}

void cgalSurf::setSurfZeroMesh()
{
    TRI* t;

    surf_tri_loop(*c_surf, t)
    {
        for (int i = 0; i < 3; i++)
        {
             if (t->side_length0[i] != -1.0) continue;
             t->side_length0[i] = separation(Point_of_tri(t)[i],
                        Point_of_tri(t)[(i+1)%3], 3);
             for (int j = 0; j < 3; j++)
                  t->side_dir0[i][j] = (Coords(Point_of_tri(t)[(i+1)%3])[j] -
                        Coords(Point_of_tri(t)[i])[j]) / t->side_length0[i];
        }
    }
    never_redistribute(Hyper_surf(*c_surf)) = YES;
}

void cgalSurf::setCurveZeroLength(CURVE* curve, double len_fac)
{
    BOND* b;

    curve_bond_loop(curve, b)
    {
        if (b->length0 != -1.0) continue;
        set_bond_length(b, 3);
        b->length0 = len_fac * bond_length(b);
        for (int i = 0; i < 3; i++)
             b->dir0[i] = (Coords(b->end)[i] - Coords(b->start)[i]) /
                        b->length0;
    }
}

void cgalSurf::setMonoCompBdryZeroLength()
{
    CURVE** c;

    surf_pos_curve_loop(*c_surf, c)
    {
        if (hsbdry_type(*c) != MONO_COMP_HSBDRY)
            continue;
        setCurveZeroLength(*c, 1.0);
    }
    surf_neg_curve_loop(*c_surf, c)
    {
        if (hsbdry_type(*c) != MONO_COMP_HSBDRY)
            continue;
        setCurveZeroLength(*c, 1.0);
    }
}

void cgalSurf::cgalGenSurf()
{
    COMPONENT neg_comp, pos_comp;
    SURFACE *newsurf;
    INTERFACE* sav_intfc = current_interface();
    CDT::Finite_vertices_iterator vit;
    CDT::Finite_faces_iterator fit;
    size_t i;

    neg_comp = pos_comp = c_intfc->default_comp;

    // set current interface to be intfc
    // the element generated by make_surface, make_curve, ... 
    // will hooked on current interface
    set_current_interface(c_intfc);
    newsurf  = make_surface(neg_comp, pos_comp, NULL, NULL);

    int num_vtx = readCDT().number_of_vertices();
    double* vertex = new double [3*num_vtx];
    std::vector<size_t> index;

    for (i = 0, vit = readCDT().finite_vertices_begin();
                vit != readCDT().finite_vertices_end(); vit++, i++)
    {
         vit->info() = i;
         index.push_back(i);
         vertex[i*3] = vit->point()[0];
         vertex[i*3+1] = vit->point()[1];
         vertex[i*3+2] = height;
    }

    std::vector<POINT*> points;
    int num_tris = 0;

    for (i = 0; i < (size_t)num_vtx; i++)
    {
         points.push_back(Point(vertex+3*i));
         points.back()->num_tris = 0;
    }
    num_tris = num_finite_face; 

    int num_point_tris = num_tris * 3;
    TRI** tris;

    uni_array(&tris, num_tris, sizeof(TRI*));
    for (i = 0, fit = readCDT().finite_faces_begin();
                fit != readCDT().finite_faces_end(); fit++, i++)
    {
         size_t i1 = index[fit->vertex(0)->info()];
         size_t i2 = index[fit->vertex(1)->info()];
         size_t i3 = index[fit->vertex(2)->info()];

         tris[i] = make_tri(points[i1], points[i2], points[i3],
                        NULL, NULL, NULL, NO);
         tris[i]->surf = newsurf;
         points[i1]->num_tris++;
         points[i2]->num_tris++;
         points[i3]->num_tris++;
    }
    c_intfc->point_tri_store = (TRI**)store((size_t)num_point_tris*sizeof(TRI*));

    TRI** ptris = c_intfc->point_tri_store;

    for (i = 0; i < (size_t)num_vtx; i++)
    {
         points[i]->tris = ptris;
         ptris += points[i]->num_tris;
         points[i]->num_tris = 0;
    }
    for (i = 0; i < (size_t)num_tris; i++)
    {
         if (i != 0)
         {
             tris[i]->prev = tris[i-1];
             tris[i-1]->next = tris[i];
         }
         for (int j = 0; j < 3; j++)
         {
              POINT* p = Point_of_tri(tris[i])[j];
              p->tris[p->num_tris++] = tris[i];
         }
    }
    for (i = 0; i < (size_t)num_vtx; i++)
    {
         TRI** tritemp = points[i]->tris;
         int num_ptris = points[i]->num_tris;
         for (int j = 0; j < num_ptris; j++)
         for (int k = 0; k < j; k++)
         {
              TRI* tri1 = tritemp[j];
              TRI* tri2 = tritemp[k];
              for (int m = 0; m < 3; m++)
              for (int l = 0; l < 3; l++)
              {
                   if (Point_of_tri(tri1)[m] == Point_of_tri(tri2)[(l+1)%3] &&
                        Point_of_tri(tri1)[(m+1)%3] == Point_of_tri(tri2)[l])
                   {
                       Tri_on_side(tri1, m) = tri2;
                       Tri_on_side(tri2, l) = tri1;
                   }
              }
         }
    }
    newsurf->num_tri = num_tris;
    first_tri(newsurf) = tris[0];
    last_tri(newsurf) = tris[num_tris-1];
    last_tri(newsurf)->next = tail_of_tri_list(newsurf);
    first_tri(newsurf)->prev = head_of_tri_list(newsurf);
    reset_intfc_num_points(newsurf->interface);
    *c_surf = newsurf;
    set_current_interface(sav_intfc);
}

void cgalRectangleSurf::getParaFromFile(std::ifstream& fin)
{
    if (!findAndLocate(fin, "Enter the height of the plane:"))
        clean_up(ERROR);
     
    double h;
 
    fin >> h;
    setHeight(h); 
    std::cout << readHeight() << std::endl;
    if (!findAndLocate(fin, "Enter lower bounds of the rectangle:"))
        clean_up(ERROR);
    fin >> lower[0] >> lower[1];
    std::cout << lower[0] << " " << lower[1] << std::endl;
    if (!findAndLocate(fin, "Enter upper bounds of the rectangle:"))
        clean_up(ERROR);
    fin >> upper[0] >> upper[1];
    std::cout << upper[0] << " " << upper[1] << std::endl;
    if (findAndLocate(fin,
                "Enter the coefficient for restricting triangular size:")) {
	double k; 

        fin >> k;
	setCGALCoeffRestricSize(k); 
    }
    std::cout << readCGALCoeffRestricSize() << std::endl;
    if (findAndLocate(fin, "Enter the bound for restricting minimum angle:")) {
	double c; 

	fin >> c;
	setCGALMinAngleUb(c); 
    } 
    std::cout << readCGALMinAngleUb() << std::endl; 
}

void cgalRectangleSurf::addCgalConst()
{
    Vertex_handle v1, v2, v3, v4;

    // add regular constraint
    v1 = insertPointToCDT(Cgal_Point(lower[0], lower[1]));
    v2 = insertPointToCDT(Cgal_Point(upper[0], lower[1]));
    v3 = insertPointToCDT(Cgal_Point(upper[0], upper[1]));
    v4 = insertPointToCDT(Cgal_Point(lower[0], upper[1]));
    insertConstraintToCDT(v1, v2);
    insertConstraintToCDT(v2, v3);
    insertConstraintToCDT(v3, v4);
    insertConstraintToCDT(v4, v1);
    // add extra constraint
    for (size_t i = 0 ; i < readExtConPoint().size(); i += 4)
    {
	 v1 = insertPointToCDT(Cgal_Point(readExtConPoint()[i], 
		readExtConPoint()[i+1]));
         v2 = insertPointToCDT(Cgal_Point(readExtConPoint()[i+2], 
		readExtConPoint()[i+3]));
         insertConstraintToCDT(v1, v2); 
    }
}

void cgalRectangleSurf::cgalTriMesh(std::ifstream& fin)
{
    CDT::Finite_faces_iterator fit;
    std::list<Cgal_Point> seeds;

    getParaFromFile(fin); 
    // add constraint
    getExtraConstPoint(fin); 
    addCgalConst(); 

    double cri_dx = readCGALCoeffRestricSize() * 
		readIntface()->table->rect_grid.h[0];

    // refine the domain by a constrained delaunay triangulation 
    // c_bound used to bound minimun angle. 
    // sin(alpha_min) = sqrt(c_bound)
    //cri_dx used to bound triangle edge size
    CGAL::refine_Delaunay_mesh_2(readCDT(), seeds.begin(), seeds.end(), 
		Criteria(readCGALMinAngleUb(), cri_dx));

    int num = readNumFinFace(); 

    for (fit = readCDT().finite_faces_begin(); 
		fit != readCDT().finite_faces_end(); fit++) 
         num++; 
    setNumFinFace(num); 
    cgalGenSurf();
    wave_type(*readSurface()) = ELASTIC_BOUNDARY;
    FT_InstallSurfEdge(*readSurface(), MONO_COMP_HSBDRY);
    setSurfZeroMesh();
    setMonoCompBdryZeroLength();
    if (consistent_interface(readIntface()) == NO)
        clean_up(ERROR);
}

const double cgalCircleSurf::eps = 1.0e-6;

double cgalCircleSurf::distance(double* p1, double* p2, int dim)
{
    double dis = 0.0; 

    for (int i = 0; i < dim; i++)
         dis += sqr(*(p1 + i) - *(p2 + i)); 

    return sqrt(dis);
}

void cgalCircleSurf::getParaFromFile(std::ifstream& fin)
{
    if (!findAndLocate(fin, "Enter the height of the plane:"))
        clean_up(ERROR);
    double h; 

    fin >> h;
    setHeight(h); 
    std::cout << readHeight() << std::endl;
    if (!findAndLocate(fin, "Enter the center of the circle:"))
        clean_up(ERROR);
    fin >> cen[0] >> cen[1];
    std::cout << cen[0] << " " << cen[1] << std::endl;
    if (!findAndLocate(fin, "Enter the radius of the circle:"))
        clean_up(ERROR);
    fin >> radius;
    std::cout << radius << std::endl;
    if (findAndLocate(fin,
                "Enter the coefficient for restricting triangular size:")) {
        double k; 

        fin >> k;
	setCGALCoeffRestricSize(k); 
    }
    std::cout << readCGALCoeffRestricSize() << std::endl;
    if (findAndLocate(fin, "Enter the bound for restricting minimum angle:")) {
	double c; 

        fin >> c;
	setCGALMinAngleUb(c); 
    }
    std::cout << readCGALMinAngleUb() << std::endl;
    if (findAndLocate(fin, "Enter the number of constraint point on circle:"))
	fin >> num_reg_const; 
    std::cout << num_reg_const << std::endl;
}

void cgalCircleSurf::addCgalConst()
{
    double theta = 2 * PI / num_reg_const; 
    Vertex_handle v1, v2; 
    std::vector<double> regConPoint; 
    
    // add regular constraint
    for (int i = 0; i < num_reg_const; i++)
    {
	 regConPoint.push_back(cen[0] + radius * cos(i * theta));
         regConPoint.push_back(cen[1] + radius * sin(i * theta));
    }
    regConPoint.push_back(cen[0] + radius);
    regConPoint.push_back(cen[1]);
    for (int i = 0; i < 2*num_reg_const; i += 2)
    {
	 v1 = insertPointToCDT(Cgal_Point(regConPoint[i], regConPoint[i+1]));
         v2 = insertPointToCDT(Cgal_Point(regConPoint[i+2], regConPoint[i+3])); 
         insertConstraintToCDT(v1, v2); 
    }
    // a test for considering C9 gore
    theta = 2 * PI / 28; 
    v1 = insertPointToCDT(Cgal_Point(cen[0], cen[1]));
    for (int i = 0; i < 28; i++) { 
	 v2 = insertPointToCDT(Cgal_Point(cen[0] + 
		radius * cos((i+0.5) * theta), cen[1] + 
		radius * sin((i+0.5) * theta))); 
	 insertConstraintToCDT(v1, v2); 
    }
    // add extra constraint
    // some extra constraint points which are very close
    // to certain regular constraint point were meant to be it
    for (size_t i = 0; i < readExtConPoint().size(); i += 2)
    {
	 double pe[2];

	 pe[0] = readExtConPoint()[i]; 
	 pe[1] = readExtConPoint()[i+1];
         for (size_t j = 0; j < regConPoint.size(); j += 2)
         {
	      double pr[2]; 
          
	      pr[0] = regConPoint[j];
	      pr[1] = regConPoint[j+1]; 
	      if (distance(pe, pr, 2) < eps)
	      {		  
		  setExtConPoint(regConPoint[j], i); 
		  setExtConPoint(regConPoint[j+1], i+1);
	      }
         }
    }
    for (size_t i = 0; i < readExtConPoint().size(); i += 4)
    {
	 v1 = insertPointToCDT(Cgal_Point(readExtConPoint()[i], 
			readExtConPoint()[i+1]));
         v2 = insertPointToCDT(Cgal_Point(readExtConPoint()[i+2], 
			readExtConPoint()[i+3])); 
         insertConstraintToCDT(v1, v2);
    }
}

void cgalCircleSurf::cgalTriMesh(std::ifstream& fin)
{
    CDT::Finite_faces_iterator fit;

    getParaFromFile(fin);
    // add constraint
    getExtraConstPoint(fin);
    addCgalConst();

    double cri_dx = readCGALCoeffRestricSize() * 
		readIntface()->table->rect_grid.h[0];
    std::list<Cgal_Point> seeds; 
    // refine the domain by a constrained delaunay triangulation 
    // c_bound used to bound minimun angle. 
    // sin(alpha_min) = sqrt(c_bound)
    // cri_dx used to bound triangle edge size
    CGAL::refine_Delaunay_mesh_2(readCDT(), seeds.begin(), seeds.end(),
                Criteria(readCGALMinAngleUb(), cri_dx));

    int num = readNumFinFace(); 

    for (fit = readCDT().finite_faces_begin(); 
		fit != readCDT().finite_faces_end(); fit++)
         num++;
    setNumFinFace(num); 
    cgalGenSurf();
    wave_type(*readSurface()) = ELASTIC_BOUNDARY;
    FT_InstallSurfEdge(*readSurface(), MONO_COMP_HSBDRY);
    setSurfZeroMesh();
    setMonoCompBdryZeroLength();
    if (consistent_interface(readIntface()) == NO)
        clean_up(ERROR);
}

void cgalParaSurf::getParaFromFile(std::ifstream& fin) {
    if (!findAndLocate(fin, "Enter the height of the canopy:"))
        clean_up(ERROR);
    
    double h; 

    fin >> h;
    setHeight(h); 
    std::cout << readHeight() << std::endl;
    if (!findAndLocate(fin, "Enter the center of the canopy:"))
        clean_up(ERROR);
    
    double c1, c2; 

    fin >> c1 >> c2;
    setCenter(c1, c2); 
    std::cout << getCenter()[0] << " " << getCenter()[1] << std::endl;
    if (!findAndLocate(fin, "Enter the radius of the canopy:"))
        clean_up(ERROR);
    double r; 

    fin >> r;
    setRadius(r); 
    std::cout << getRadius() << std::endl;
    if (findAndLocate(fin,
                "Enter the coefficient for restricting triangular size:")) {
	double k;
 
        fin >> k;
	setCGALCoeffRestricSize(k); 
    }
    std::cout << readCGALCoeffRestricSize() << std::endl;
    if (findAndLocate(fin, "Enter the bound for restricting minimum angle:")) {
	double c; 

        fin >> c;
	setCGALMinAngleUb(c); 
    }
    std::cout << readCGALMinAngleUb() << std::endl;
    if (!findAndLocate(fin, "Enter the number of gores:"))
	std::runtime_error("Must specify the number of gores!\n"); 
    fin >> num_lines; 
}

void cgalParaSurf::addCgalConst() {
    setNumRegConst(num_lines * 5); 

    double theta = 2 * PI / (getNumRegConst());
    Vertex_handle v1, v2;
    Vertex_handle *v = new Vertex_handle [getNumRegConst()]; 
    std::vector< std::pair<double, double> > regConPoint;
  
    for (int i = 0; i < getNumRegConst(); i++)
         regConPoint.push_back(std::make_pair(getCenter()[0] + 
		getRadius() * cos(i * theta), getCenter()[1] + 
		getRadius() * sin(i * theta)));
    regConPoint.push_back(std::make_pair(getCenter()[0] + getRadius(), 
		getCenter()[1]));
    for (int i = 0; i < getNumRegConst(); i++)
    {
         v1 = insertPointToCDT(Cgal_Point(regConPoint[i].first, 
		regConPoint[i].second));
         v2 = insertPointToCDT(Cgal_Point(regConPoint[i+1].first, 
		regConPoint[i+1].second));
         insertConstraintToCDT(v1, v2);
	 v[i] = v1; 
    }
    v1 = insertPointToCDT(Cgal_Point(getCenter()[0], getCenter()[1]));
    for (int i = 0; i < num_lines; i++) {
         insertConstraintToCDT(v1, v[i*5]);
    }
    delete [] v; 
    std::ofstream fout("points.txt"); 

    for (int i = 0; i < num_lines; i++) {
	 fout << regConPoint[i*5].first << ' ' << regConPoint[i*5].second 
		<< ' ' << readHeight() << std::endl; 
    }
    for (int i = 0; i < num_lines + 1; i++) {
         fout << 0 << ' ' << i + 1 << std::endl;
    }
}