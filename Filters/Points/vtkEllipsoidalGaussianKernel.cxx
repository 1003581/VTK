/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEllipsoidalGaussianKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEllipsoidalGaussianKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkEllipsoidalGaussianKernel);

//----------------------------------------------------------------------------
vtkEllipsoidalGaussianKernel::vtkEllipsoidalGaussianKernel()
{
  this->UseNormals = true;
  this->UseScalars = false;

  this->NormalsArrayName = "Normals";
  this->ScalarsArrayName = "Scalars";

  this->ScaleFactor = 1.0;
  this->Radius = 1.0;
  this->Sharpness = 2.0;
  this->Eccentricity = 2.0;

  this->F2 = this->Sharpness / this->Radius;
  this->E2 = this->Eccentricity * this->Eccentricity;
  this->NormalsArray = NULL;
  this->ScalarsArray = NULL;
}


//----------------------------------------------------------------------------
vtkEllipsoidalGaussianKernel::~vtkEllipsoidalGaussianKernel()
{
  this->FreeStructures();
}


//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::
FreeStructures()
{
  this->Superclass::FreeStructures();

  if ( this->NormalsArray )
    {
    this->NormalsArray->Delete();
    this->NormalsArray = NULL;
    }

  if ( this->ScalarsArray )
    {
    this->ScalarsArray->Delete();
    this->ScalarsArray = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *pd)
{
  this->Superclass::Initialize(loc, ds, pd);

  this->ScalarsArray = pd->GetArray(this->ScalarsArrayName);
  if ( this->UseScalars && this->ScalarsArray &&
       this->ScalarsArray->GetNumberOfComponents() == 1 )
    {
    this->ScalarsArray->Register(this);
    }
  else
    {
    this->ScalarsArray = NULL;
    }

  this->NormalsArray = pd->GetArray(this->NormalsArrayName);
  if ( this->UseNormals && this->NormalsArray )
    {
    this->NormalsArray->Register(this);
    }
  else
    {
    this->NormalsArray = NULL;
    }

  this->F2 = this->Sharpness / this->Radius;
  this->F2 = this->F2 * this->F2;
  this->E2 = this->Eccentricity * this->Eccentricity;
}

//----------------------------------------------------------------------------
vtkIdType vtkEllipsoidalGaussianKernel::
ComputeBasis(double x[3], vtkIdList *pIds)
{
  this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  return pIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
vtkIdType vtkEllipsoidalGaussianKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double sum = 0.0;
  weights->SetNumberOfTuples(numPts);
  double *w = weights->GetPointer(0);
  double y[3], v[3], r2, z2, rxy2, mag;
  double n[3], s;
  double f2=this->F2, e2=this->E2;

  for (i=0; i<numPts; ++i)
    {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);

    v[0] = x[0] - y[0];
    v[1] = x[1] - y[1];
    v[2] = x[2] - y[2];
    r2 = vtkMath::Dot(v,v);

    if ( r2 == 0.0 ) //precise hit on existing point
      {
      pIds->SetNumberOfIds(1);
      pIds->SetId(0,id);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0,1.0);
      return 1;
      }
    else // continue computing weights
      {
      // Normal affect
      if ( this->NormalsArray )
        {
        this->NormalsArray->GetTuple(id,n);
        mag = vtkMath::Dot(n,n);
        mag = ( mag == 0.0 ? 1.0 : sqrt(mag) );
        }
      else
        {
        mag = 1.0;
        }

      // Scalar scaling
      if ( this->ScalarsArray )
        {
        this->ScalarsArray->GetTuple(id,&s);
        }
      else
        {
        s = 1.0;
        }

      z2 = vtkMath::Dot(v,n) / mag;
      z2 = z2*z2;
      rxy2 = r2 - z2;

      w[i] = s * exp(-f2 * (rxy2/e2 + z2));
      sum += w[i];
      }//computing weights
    }//over all points

  // Normalize
  for (i=0; i<numPts; ++i)
    {
    w[i] /= sum;
    }

  return numPts;
}

//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Use Normals: "
     << (this->UseNormals? "On" : " Off") << "\n";
  os << indent << "Use Scalars: "
     << (this->UseScalars? "On" : " Off") << "\n";

  os << indent << "Scalars Array Name: " << this->ScalarsArrayName << "\n";
  os << indent << "Normals Array Name: " << this->NormalsArrayName << "\n";

  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "Sharpness: " << this->Sharpness << endl;
  os << indent << "Eccentricity: " << this->Eccentricity << endl;

}
