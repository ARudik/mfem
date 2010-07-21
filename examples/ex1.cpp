//                                MFEM Example 1
//
// Compile with: make ex1
//
// Sample runs:  ex1 square-disc.mesh2d
//               ex1 star.mesh2d
//               ex1 escher.mesh3d
//               ex1 fichera.mesh3d
//
// Description: This example code demostrates the use of MFEM to define a simple
//              linear finite element discretization of the Laplace problem
//              -Delta u = 1 with homegenous Dirichlet boundary conditions.
//
//              The example highlights the use of mesh refinement, finite
//              element grid functions, as well as linear and bilinear forms
//              corresponding to the left-hand side and right-hand side of the
//              discrete linear system. We also cover the explicit elimination
//              of boundary conditions on all boundary edges, and the optional
//              connection to the GLVis tool for visualization.

#include <fstream>
#include "mfem.hpp"

int main (int argc, char *argv[])
{
   Mesh *mesh;

   if (argc == 1)
   {
      cout << "Usage: ex1 <mesh_file>" << endl;
      return 1;
   }

   // 1. Read the mesh from the given mesh file. We can handle triangular,
   //    qualirateral, tetrahedral or hexahedral elements with the same code.
   ifstream imesh(argv[1]);
   if (!imesh)
   {
      cerr << "can not open mesh file: " << argv[1] << endl;
      return 2;
   }
   mesh = new Mesh(imesh, 1, 1);
   imesh.close();

   // 2. Refine the mesh to increase the resolution. In this example we do
   //    'ref_levels' of uniform refinement. We choose 'ref_levels' to be the
   //    largest number that gives a final mesh with no more than 50,000
   //    elements.
   {
      int ref_levels =
         (int)floor(log(50000./mesh->GetNE())/log(2.)/mesh->Dimension());
      for (int l = 0; l < ref_levels; l++)
         mesh->UniformRefinement();
   }

   // 3. Define a finite element space on the mesh. Here we use linear finite
   //    elements.
   FiniteElementCollection *fec = new LinearFECollection;
   FiniteElementSpace *fespace = new FiniteElementSpace(mesh, fec);

   // 4. Set up the linear form b(.) which corresponds to the right-hand side of
   //    the FEM linear system, which in this case is (1,phi_i) where phi_i are
   //    the basis functions in the finite element espace.
   LinearForm *b = new LinearForm(fespace);
   ConstantCoefficient one(1.0);
   b->AddDomainIntegrator(new DomainLFIntegrator(one));
   b->Assemble();

   // 5. Define the solution vector x as a finite element grid function
   //    corresponding to fespace.  Initialize x with inital guess of zero,
   //    which satisfies the boundary conditions.
   GridFunction x(fespace);
   x = 0.0;

   // 6. Set up the bilinear form a(.,.) on the finite element space
   //    corresponding to the Laplacian operator -Delta, by adding the Diffusion
   //    domain integrator and imposing homegenous Dirichlet boundary
   //    conditions.  The boundary conditions are implemented by marking all the
   //    boundary attributes from the mesh as essential (Dirichlet). After
   //    assembly and finalizing we extract the corresponding sparse matrix A.
   BilinearForm *a = new BilinearForm(fespace);
   a->AddDomainIntegrator(new DiffusionIntegrator(one));
   a->Assemble();
   Array<int> ess_bdr(mesh->bdr_attributes.Size());
   ess_bdr = 1;
   a->EliminateEssentialBC(ess_bdr, x, *b);
   a->Finalize();
   const SparseMatrix &A = a->SpMat();

   // 7. Define a simple symmetric Gauss-Seidel preconditioner and use it to
   //    solve the system Ax=b with PCG.
   GSSmoother M(A);
   PCG(A, M, *b, x, 1, 200, 1e-12, 1e-28);

   // 8. Save the solution to a file (as a finite element grid function). This
   //    can be viewed later using "glvis -m <mesh_file> -g sol.gf".
   ofstream sol_ofs("sol.gf");
   x.Save(sol_ofs);

   // 9. (Optional) Send the solution by socket to a GLVis server
   char vishost[] = "localhost";
   int  visport   = 19916;
   osockstream sol_sock (visport, vishost);
   if (mesh->Dimension() == 2)
      sol_sock << "fem2d_gf_data\n";
   else
      sol_sock << "fem3d_gf_data\n";
   mesh->Print(sol_sock);
   x.Save(sol_sock);
   sol_sock.send();

   // 10. Free the used memory
   delete a;
   delete b;
   delete fespace;
   delete fec;
   delete mesh;
}