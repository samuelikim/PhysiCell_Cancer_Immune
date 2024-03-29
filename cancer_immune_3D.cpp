/*
###############################################################################
# If you use PhysiCell in your project, please cite PhysiCell and the version #
# number, such as below:                                                      #
#                                                                             #
# We implemented and solved the model using PhysiCell (Version x.y.z) [1].    #
#                                                                             #
# [1] A Ghaffarizadeh, R Heiland, SH Friedman, SM Mumenthaler, and P Macklin, #
#     PhysiCell: an Open Source Physics-Based Cell Simulator for Multicellu-  #
#     lar Systems, PLoS Comput. Biol. 14(2): e1005991, 2018                   #
#     DOI: 10.1371/journal.pcbi.1005991                                       #
#                                                                             #
# See VERSION.txt or call get_PhysiCell_version() to get the current version  #
#     x.y.z. Call display_citations() to get detailed information on all cite-#
#     able software used in your PhysiCell application.                       #
#                                                                             #
# Because PhysiCell extensively uses BioFVM, we suggest you also cite BioFVM  #
#     as below:                                                               #
#                                                                             #
# We implemented and solved the model using PhysiCell (Version x.y.z) [1],    #
# with BioFVM [2] to solve the transport equations.                           #
#                                                                             #
# [1] A Ghaffarizadeh, R Heiland, SH Friedman, SM Mumenthaler, and P Macklin, #
#     PhysiCell: an Open Source Physics-Based Cell Simulator for Multicellu-  #
#     lar Systems, PLoS Comput. Biol. 14(2): e1005991, 2018                   #
#     DOI: 10.1371/journal.pcbi.1005991                                       #
#                                                                             #
# [2] A Ghaffarizadeh, SH Friedman, and P Macklin, BioFVM: an efficient para- #
#     llelized diffusive transport solver for 3-D biological simulations,     #
#     Bioinformatics 32(8): 1256-8, 2016. DOI: 10.1093/bioinformatics/btv730  #
#                                                                             #
###############################################################################
#                                                                             #
# BSD 3-Clause License (see https://opensource.org/licenses/BSD-3-Clause)     #
#                                                                             #
# Copyright (c) 2015-2021, Paul Macklin and the PhysiCell Project             #
# All rights reserved.                                                        #
#                                                                             #
# Redistribution and use in source and binary forms, with or without          #
# modification, are permitted provided that the following conditions are met: #
#                                                                             #
# 1. Redistributions of source code must retain the above copyright notice,   #
# this list of conditions and the following disclaimer.                       #
#                                                                             #
# 2. Redistributions in binary form must reproduce the above copyright        #
# notice, this list of conditions and the following disclaimer in the         #
# documentation and/or other materials provided with the distribution.        #
#                                                                             #
# 3. Neither the name of the copyright holder nor the names of its            #
# contributors may be used to endorse or promote products derived from this   #
# software without specific prior written permission.                         #
#                                                                             #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" #
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   #
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  #
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE   #
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR         #
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF        #
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    #
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN     #
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)     #
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  #
# POSSIBILITY OF SUCH DAMAGE.                                                 #
#                                                                             #
###############################################################################
*/

#include "./cancer_immune_3D.h"

Cell_Definition* pImmuneCell;
Cell_Definition* pMacrophage;


// 2/21/2022 Set up the PhysiCell mechanics data structure.
    // Set mechanics voxel size.
double mechanics_voxel_size = 30;
Cell_Container* cell_container = create_cell_container_for_microenvironment(
    microenvironment, mechanics_voxel_size );


void create_immune_cell_type( void )
{
    pImmuneCell = find_cell_definition( "immune cell" );
    
    static int oxygen_ID = microenvironment.find_density_index( "oxygen" );
    static int immuno_ID = microenvironment.find_density_index( "immunostimulatory factor" );
    
    // reduce o2 uptake
    
    pImmuneCell->phenotype.secretion.uptake_rates[oxygen_ID] *=
        parameters.doubles("immune_o2_relative_uptake");
    
    pImmuneCell->phenotype.mechanics.cell_cell_adhesion_strength *=
        parameters.doubles("immune_relative_adhesion");
    pImmuneCell->phenotype.mechanics.cell_cell_repulsion_strength *=
        parameters.doubles("immune_relative_repulsion");
        
    // figure out mechanics parameters
    
    pImmuneCell->phenotype.mechanics.relative_maximum_attachment_distance
        = pImmuneCell->custom_data["max_attachment_distance"] / pImmuneCell->phenotype.geometry.radius ;
        
    pImmuneCell->phenotype.mechanics.attachment_elastic_constant
        = pImmuneCell->custom_data["elastic_coefficient"];
    
    pImmuneCell->phenotype.mechanics.relative_detachment_distance
        = pImmuneCell->custom_data["max_attachment_distance" ] / pImmuneCell->phenotype.geometry.radius ;
    
    // set functions
    
    pImmuneCell->functions.update_phenotype = NULL;
    pImmuneCell->functions.custom_cell_rule = immune_cell_rule;
    pImmuneCell->functions.update_migration_bias = immune_cell_motility;
    pImmuneCell->functions.contact_function = adhesion_contact_function;
    
    // set custom data values

    return;
}

// create macrophages, which will eventually diff into M1 or M2
void create_macrophage_type( void )
{
    pMacrophage = find_cell_definition( "macrophage" );
    
    static int oxygen_ID = microenvironment.find_density_index( "oxygen" );
    /*
    static int IFNg_ID = microenvironment.find_density_index( "IFNg" );
    static int IL4_ID = microenvironment.find_density_index( "IL4" );
    */
    // reduce o2 uptake (same as immune cell, aka T cell)
    
    pMacrophage->phenotype.secretion.uptake_rates[oxygen_ID] *= parameters.doubles("immune_o2_relative_uptake");
    /*
    pMacrophage->phenotype.mechanics.cell_cell_adhesion_strength *=
        parameters.doubles("immune_relative_adhesion");
    pMacrophage->phenotype.mechanics.cell_cell_repulsion_strength *=
        parameters.doubles("immune_relative_repulsion");
    
    // figure out mechanics parameters
    
    pMacrophage->phenotype.mechanics.relative_maximum_attachment_distance
        = pMacrophage->custom_data["max_attachment_distance"] / pMacrophage->phenotype.geometry.radius ;
        
    pMacrophage->phenotype.mechanics.attachment_elastic_constant
        = pMacrophage->custom_data["elastic_coefficient"];
    
    pMacrophage->phenotype.mechanics.relative_detachment_distance
        = pMacrophage->custom_data["max_attachment_distance" ] / pMacrophage->phenotype.geometry.radius ;
    */
    // set functions
    
    pMacrophage->functions.update_phenotype = NULL;
    pMacrophage->functions.custom_cell_rule = macrophage_rule;
    pMacrophage->functions.update_migration_bias = macrophage_motility;
    // pMacrophage->functions.contact_function = macrophage_function;
    
    // set custom data values

    return;
}




void create_cell_types( void )
{
    // use the same random seed so that future experiments have the
    // same initial histogram of oncoprotein, even if threading means
    // that future division and other events are still not identical
    // for all runs
    SeedRandom( parameters.ints("random_seed") );
    
    // housekeeping
    
    initialize_default_cell_definition();
    
    // messing around w/ o2 saturation and threshold levels to get the desired hypoxic core
    cell_defaults.parameters.o2_proliferation_saturation = 38.0;
    cell_defaults.parameters.o2_proliferation_threshold = 1.0;
    cell_defaults.parameters.o2_reference = 38.0;
    // setting necrosis threshold and saturation
    cell_defaults.parameters.o2_necrosis_max = 30;
    cell_defaults.parameters.o2_necrosis_threshold = 30;


    static int oxygen_ID = microenvironment.find_density_index( "oxygen" ); // 0
    static int immuno_ID = microenvironment.find_density_index( "immunostimulatory factor" ); // 1
    
    /*
       This parses the cell definitions in the XML config file.
    */

    initialize_cell_definitions_from_pugixml();
    
    
    // change the max cell-cell adhesion distance
    cell_defaults.phenotype.mechanics.relative_maximum_attachment_distance =
        cell_defaults.custom_data["max_attachment_distance"] / cell_defaults.phenotype.geometry.radius;
        
    cell_defaults.phenotype.mechanics.relative_detachment_distance
        = cell_defaults.custom_data["max_attachment_distance"] / cell_defaults.phenotype.geometry.radius ;
        
    cell_defaults.phenotype.mechanics.attachment_elastic_constant
        = cell_defaults.custom_data[ "elastic_coefficient" ];
        
    cell_defaults.functions.update_phenotype = tumor_cell_phenotype_with_and_immune_stimulation;
    cell_defaults.functions.custom_cell_rule = NULL;
    
    cell_defaults.functions.contact_function = adhesion_contact_function;
    cell_defaults.functions.update_migration_bias = NULL;

    // create the immune cell type
    create_immune_cell_type();

    // create macrophage cell type
    create_macrophage_type();
    
    build_cell_definitions_maps();
    display_cell_definitions( std::cout );
    
    return;
}

void setup_microenvironment( void )
{
    
    if( default_microenvironment_options.simulate_2D == true )
    {
        std::cout << "Warning: overriding 2D setting to return to 3D" << std::endl;
        default_microenvironment_options.simulate_2D = false;
    }
    
    initialize_microenvironment();

    return;
}

/*
void introduce_immune_cells( void )
{
    double tumor_radius = -9e9; // 250.0;
    double temp_radius = 0.0;
    
    // for the loop, deal with the (faster) norm squared
    for( int i=0; i < (*all_cells).size() ; i++ )
    {
        temp_radius = norm_squared( (*all_cells)[i]->position );
        if( temp_radius > tumor_radius )
        { tumor_radius = temp_radius; }
    }
    // now square root to get to radius
    tumor_radius = sqrt( tumor_radius );
    
    // if this goes wackadoodle, choose 250
    if( tumor_radius < 250.0 )
    { tumor_radius = 250.0; }
    
    std::cout << "current tumor radius: " << tumor_radius << std::endl;
    
    // now seed immune cells
    
    int number_of_immune_cells =
        parameters.ints("number_of_immune_cells"); // 7500; // 100; // 40;
    double radius_inner = tumor_radius +
        parameters.doubles("initial_min_immune_distance_from_tumor"); 30.0; // 75 // 50;
    double radius_outer = radius_inner +
        parameters.doubles("thickness_of_immune_seeding_region"); // 75.0; // 100; // 1000 - 50.0;
    
    double mean_radius = 0.5*(radius_inner + radius_outer);
    double std_radius = 0.33*( radius_outer-radius_inner)/2.0;
    
    for( int i=0 ;i < number_of_immune_cells ; i++ )
    {
        double theta = UniformRandom() * 6.283185307179586476925286766559;
        double phi = acos( 2.0*UniformRandom() - 1.0 );
        
        double radius = NormalRandom( mean_radius, std_radius );
        
        Cell* pCell = create_cell( *pImmuneCell );
        pCell->assign_position( radius*cos(theta)*sin(phi), radius*sin(theta)*sin(phi), radius*cos(phi) );
    }
    
    return;
}
*/

std::vector<std::vector<double>> create_cell_sphere_positions(double cell_radius, double sphere_radius)
{
    std::vector<std::vector<double>> cells;
    int xc=0,yc=0,zc=0;
    double x_spacing= cell_radius*sqrt(3);
    double y_spacing= cell_radius*2;
    double z_spacing= cell_radius*sqrt(3);
    
    std::vector<double> tempPoint(3,0.0);
    // std::vector<double> cylinder_center(3,0.0);
    
    for(double z=-sphere_radius;z<sphere_radius;z+=z_spacing, zc++)
    {
        for(double x=-sphere_radius;x<sphere_radius;x+=x_spacing, xc++)
        {
            for(double y=-sphere_radius;y<sphere_radius;y+=y_spacing, yc++)
            {
                tempPoint[0]=x + (zc%2) * 0.5 * cell_radius;
                tempPoint[1]=y + (xc%2) * cell_radius;
                tempPoint[2]=z;
                
                if(sqrt(norm_squared(tempPoint))< sphere_radius)
                { cells.push_back(tempPoint); }
            }
            
        }
    }
    return cells;
    
}

void setup_tissue( void )
{
    // this doesn't really work, keep it in just in case
    /*
    // setup tissue by placing a few tumor cells that divide rapidly (rather than cluster of tumor cells at center)
    
    double Xmin = microenvironment.mesh.bounding_box[0];
    double Ymin = microenvironment.mesh.bounding_box[1];
    double Zmin = microenvironment.mesh.bounding_box[2];

    double Xmax = microenvironment.mesh.bounding_box[3];
    double Ymax = microenvironment.mesh.bounding_box[4];
    double Zmax = microenvironment.mesh.bounding_box[5];
    
    /*
    if( default_microenvironment_options.simulate_2D == true )
    {
        Zmin = 0.0;
        Zmax = 0.0;
    }
    
    
    double Xrange = Xmax - Xmin;
    double Yrange = Ymax - Ymin;
    double Zrange = Zmax - Zmin;
    
    // define cancer cell and mean immunogenicity and standard deviation
    
    Cell_Definition* pCell = find_cell_definition( "cancer cell" );
    static double imm_mean = parameters.doubles("tumor_mean_immunogenicity");
    static double imm_sd = parameters.doubles("tumor_immunogenicity_standard_deviation");
    
    // create random tumor cells
    std::cout << "Placing cells of type " << pCell->name << " ... " << std::endl;
    for( int n = 0 ; n < parameters.ints("number_of_cells") ; n++ )
    {
        std::vector<double> position = {0,0,0};
        position[0] = Xmin + UniformRandom()*Xrange;
        position[1] = Ymin + UniformRandom()*Yrange;
        position[2] = Zmin + UniformRandom()*Zrange;
            
        Cell* pC = create_cell( *pCell );
        pC->assign_position( position );
        
        pCell->custom_data["oncoprotein"] = NormalRandom( imm_mean, imm_sd );
        if( pCell->custom_data["oncoprotein"] < 0.0 )
        { pCell->custom_data["oncoprotein"] = 0.0; }
    }
    
    std::cout << std::endl;
    
    // load cells from your CSV file (if enabled)
    load_cells_from_pugixml();
    return;
    */

    // place a cluster of tumor cells at the center
    
    double cell_radius = cell_defaults.phenotype.geometry.radius;
    double cell_spacing = 0.95 * 2.0 * cell_radius;
    
    double tumor_radius =
        parameters.doubles("tumor_radius"); // 250.0;
    
    Cell* pCell = NULL;
    // for macrophage
    Cell* pC = NULL;
    
    std::vector<std::vector<double>> positions = create_cell_sphere_positions(cell_radius,tumor_radius);
    std::cout << "creating " << positions.size() << " closely-packed tumor cells ... " << std::endl;
    
    static double imm_mean = parameters.doubles("tumor_mean_immunogenicity");
    static double imm_sd = parameters.doubles("tumor_immunogenicity_standard_deviation");
        
    for( int i=0; i < positions.size(); i++ )
    {
        pCell = create_cell(); // tumor cell
        pCell->assign_position( positions[i] );
        pCell->custom_data["oncoprotein"] = NormalRandom( imm_mean, imm_sd );
        if( pCell->custom_data["oncoprotein"] < 0.0 )
        { pCell->custom_data["oncoprotein"] = 0.0; }
    }
    
    double sum = 0.0;
    double min = 9e9;
    double max = -9e9;
    for( int i=0; i < all_cells->size() ; i++ )
    {
        double r = (*all_cells)[i]->custom_data["oncoprotein"];
        sum += r;
        if( r < min )
        { min = r; }
        if( r > max )
        { max = r; }
    }
    double mean = sum / ( all_cells->size() + 1e-15 );
    // compute standard deviation
    sum = 0.0;
    for( int i=0; i < all_cells->size(); i++ )
    {
        sum +=  ( (*all_cells)[i]->custom_data["oncoprotein"] - mean )*( (*all_cells)[i]->custom_data["oncoprotein"] - mean );
    }
    double standard_deviation = sqrt( sum / ( all_cells->size() - 1.0 + 1e-15 ) );
    
    std::cout << std::endl << "Oncoprotein summary: " << std::endl
              << "===================" << std::endl;
    std::cout << "mean: " << mean << std::endl;
    std::cout << "standard deviation: " << standard_deviation << std::endl;
    std::cout << "[min max]: [" << min << " " << max << "]" << std::endl << std::endl;
    
    // 3/8 seed in macrophages randomly throughout the tumor core
    for (int i = 0; i < parameters.ints("number_of_initial_macrophages"); i++)
    {
        std::vector<double> macrophage_position = {0,0,0};
        
        // create random r, theta, phi that will lead to uniformly scattered macrophages
        double r = sqrt(UniformRandom()) * tumor_radius;
        double theta = UniformRandom() * 6.2831853;
        double phi = acos( 2.0*UniformRandom() - 1.0 );
        // create and assign positions
        macrophage_position[0] = r*cos(theta)*sin(phi);
        macrophage_position[1] = r*sin(theta)*sin(phi);
        macrophage_position[2] = r*cos(phi);
        pC = create_cell(*pMacrophage);
        pC -> assign_position(macrophage_position);
        
    }
    
    
    return;
     
    
    
}

// custom cell phenotype function to scale immunostimulatory factor with hypoxia
void tumor_cell_phenotype_with_and_immune_stimulation( Cell* pCell, Phenotype& phenotype, double dt )
{
    static int cycle_start_index = live.find_phase_index( PhysiCell_constants::live );
    static int cycle_end_index = live.find_phase_index( PhysiCell_constants::live );
    static int oncoprotein_i = pCell->custom_data.find_variable_index( "oncoprotein" );
    
    // update secretion rates based on hypoxia
    
    static int o2_index = microenvironment.find_density_index( "oxygen" );
    static int immune_factor_index = microenvironment.find_density_index( "immunostimulatory factor" );
    double o2 = pCell->nearest_density_vector()[o2_index];

    phenotype.secretion.secretion_rates[immune_factor_index] = 10.0;
    
    update_cell_and_death_parameters_O2_based(pCell,phenotype,dt);
    
    // if cell is dead, don't bother with future phenotype changes.
    // set it to secrete the immunostimulatory factor
    if( phenotype.death.dead == true )
    {
        phenotype.secretion.secretion_rates[immune_factor_index] = 10;
        pCell->functions.update_phenotype = NULL;
        return;
    }
    
    // if cell is attached to immune cell but doesn't die, become PDL1+ which (in another function) will turn future death rate to 0
    if( pCell->state.attached_cells.size() > 0 && pCell->phenotype.death.dead == false)
    {
        pCell -> custom_data["PDL1"] = 0; // 0 corresponds to PDL1+, 1 to PDL1-
    }


    // multiply proliferation rate by the oncoprotein
    phenotype.cycle.data.transition_rate( cycle_start_index ,cycle_end_index ) *= pCell->custom_data[oncoprotein_i] ;
    
    return;
}

std::vector<std::string> cancer_immune_coloring_function( Cell* pCell )
{
    static int oncoprotein_i = pCell->custom_data.find_variable_index( "oncoprotein" );
    
    // immune are black
    std::vector< std::string > output( 4, "black" );
    
    if( pCell->type == 1 )
    {
        output[0] = "lime";
        output[1] = "lime";
        output[2] = "green";
        return output;
    }
    
    // macrophages are gold
    if ( pCell->type == 2)
    {
        output[0] = "gold";
        output[1] = "gold";
        output[2] = "gold";
        return output;
    }

    // if I'm under attack, color me
    if( pCell->state.attached_cells.size() > 0 )
    {
        output[0] = "darkcyan"; // orangered // "purple"; // 128,0,128
        output[1] = "black"; // "magenta";
        output[2] = "cyan"; // "magenta"; //255,0,255
        return output;
    }
    
    // CUSTOM
    // if cell is attacked but survives, turn pink and STAY PINK
    if( (pCell->state.attached_cells.size() > 0 && pCell->phenotype.death.dead == false) || (pCell->custom_data["PDL1"] == 0) )
    {
        output[0]="hotpink";
        output[1]="hotpink";
        output[2]="hotpink";
        return output;
    }
    
    // live cells are green, but shaded by oncoprotein value
    if( pCell->phenotype.death.dead == false )
    {
        int oncoprotein = (int) round( 0.5 * pCell->custom_data[oncoprotein_i] * 255.0 );
        char szTempString [128];
        sprintf( szTempString , "rgb(%u,%u,%u)", oncoprotein, oncoprotein, 255-oncoprotein );
        output[0].assign( szTempString );
        output[1].assign( szTempString );

        sprintf( szTempString , "rgb(%u,%u,%u)", (int)round(output[0][0]/2.0) , (int)round(output[0][1]/2.0) , (int)round(output[0][2]/2.0) );
        output[2].assign( szTempString );
        
        return output;
    }

    // if not, dead colors
    
    if (pCell->phenotype.cycle.current_phase().code == PhysiCell_constants::apoptotic )  // Apoptotic - Red
    {
        output[0] = "rgb(255,0,0)";
        output[2] = "rgb(125,0,0)";
    }
    
    // Necrotic - Brown
    if( pCell->phenotype.cycle.current_phase().code == PhysiCell_constants::necrotic_swelling ||
        pCell->phenotype.cycle.current_phase().code == PhysiCell_constants::necrotic_lysed ||
        pCell->phenotype.cycle.current_phase().code == PhysiCell_constants::necrotic )
    {
        output[0] = "rgb(250,138,38)";
        output[2] = "rgb(139,69,19)";
    }
    return output;
}

/*
void add_elastic_velocity( Cell* pActingOn, Cell* pAttachedTo , double elastic_constant )
{
    std::vector<double> displacement = pAttachedTo->position - pActingOn->position;
    axpy( &(pActingOn->velocity) , elastic_constant , displacement );
    
    return;
}

void extra_elastic_attachment_mechanics( Cell* pCell, Phenotype& phenotype, double dt )
{
    for( int i=0; i < pCell->state.attached_cells.size() ; i++ )
    {
        add_elastic_velocity( pCell, pCell->state.attached_cells[i], pCell->custom_data["elastic_coefficient"] );
    }

    return;
}

void attach_cells( Cell* pCell_1, Cell* pCell_2 )
{
    #pragma omp critical
    {
        
    bool already_attached = false;
    for( int i=0 ; i < pCell_1->state.attached_cells.size() ; i++ )
    {
        if( pCell_1->state.attached_cells[i] == pCell_2 )
        { already_attached = true; }
    }
    if( already_attached == false )
    { pCell_1->state.attached_cells.push_back( pCell_2 ); }
    
    already_attached = false;
    for( int i=0 ; i < pCell_2->state.attached_cells.size() ; i++ )
    {
        if( pCell_2->state.attached_cells[i] == pCell_1 )
        { already_attached = true; }
    }
    if( already_attached == false )
    { pCell_2->state.attached_cells.push_back( pCell_1 ); }

    }

    return;
}

void dettach_cells( Cell* pCell_1 , Cell* pCell_2 )
{
    #pragma omp critical
    {
        bool found = false;
        int i = 0;
        while( !found && i < pCell_1->state.attached_cells.size() )
        {
            // if cell 2 is in cell 1's list, remove it
            if( pCell_1->state.attached_cells[i] == pCell_2 )
            {
                int n = pCell_1->state.attached_cells.size();
                // copy last entry to current position
                pCell_1->state.attached_cells[i] = pCell_1->state.attached_cells[n-1];
                // shrink by one
                pCell_1->state.attached_cells.pop_back();
                found = true;
            }
            i++;
        }
    
        found = false;
        i = 0;
        while( !found && i < pCell_2->state.attached_cells.size() )
        {
            // if cell 1 is in cell 2's list, remove it
            if( pCell_2->state.attached_cells[i] == pCell_1 )
            {
                int n = pCell_2->state.attached_cells.size();
                // copy last entry to current position
                pCell_2->state.attached_cells[i] = pCell_2->state.attached_cells[n-1];
                // shrink by one
                pCell_2->state.attached_cells.pop_back();
                found = true;
            }
            i++;
        }

    }
    
    return;
}
*/

void immune_cell_motility( Cell* pCell, Phenotype& phenotype, double dt )
{
    // if attached, biased motility towards director chemoattractant
    // otherwise, biased motility towards cargo chemoattractant
    
    static int immune_factor_index = microenvironment.find_density_index( "immunostimulatory factor" );

    // if not docked, attempt biased chemotaxis
    if( pCell->state.attached_cells.size() == 0 )
    {
        phenotype.motility.is_motile = true;
        
        phenotype.motility.migration_bias_direction = pCell->nearest_gradient(immune_factor_index);
        normalize( &( phenotype.motility.migration_bias_direction ) );
    }
    else
    {
        phenotype.motility.is_motile = false;
    }
    
    return;
}

Cell* immune_cell_check_neighbors_for_attachment( Cell* pAttacker , double dt )
{
    std::vector<Cell*> nearby = pAttacker->cells_in_my_container();
    int i = 0;
    while( i < nearby.size() )
    {
        // don't try to kill yourself
        if( nearby[i] != pAttacker )
        {
            if( immune_cell_attempt_attachment( pAttacker, nearby[i] , dt ) )
            { return nearby[i]; }
        }
        i++;
    }
    
    return NULL;
}

bool immune_cell_attempt_attachment( Cell* pAttacker, Cell* pTarget , double dt )
{
    static int oncoprotein_i = pTarget->custom_data.find_variable_index( "oncoprotein" );
    static int attach_rate_i = pAttacker->custom_data.find_variable_index( "attachment_rate" );

    double oncoprotein_saturation =
        pAttacker->custom_data["oncoprotein_saturation"];
    double oncoprotein_threshold =
        pAttacker->custom_data["oncoprotein_threshold"];
    double oncoprotein_difference = oncoprotein_saturation - oncoprotein_threshold;
    
    double max_attachment_distance =
        pAttacker->custom_data["max_attachment_distance"];
    double min_attachment_distance =
        pAttacker->custom_data["min_attachment_distance"];
    double attachment_difference = max_attachment_distance - min_attachment_distance;
    
    if( pTarget->custom_data[oncoprotein_i] > oncoprotein_threshold && pTarget->phenotype.death.dead == false )
    {
        std::vector<double> displacement = pTarget->position - pAttacker->position;
        double distance_scale = norm( displacement );
        if( distance_scale > max_attachment_distance )
        { return false; }
    
        double scale = pTarget->custom_data[oncoprotein_i];
        scale -= oncoprotein_threshold;
        scale /= oncoprotein_difference;
        if( scale > 1.0 )
        { scale = 1.0; }
        
        distance_scale *= -1.0;
        distance_scale += max_attachment_distance;
        distance_scale /= attachment_difference;
        if( distance_scale > 1.0 )
        { distance_scale = 1.0; }
        
        if( UniformRandom() < pAttacker->custom_data[attach_rate_i] * scale * dt * distance_scale )
        {
//            std::cout << "\t attach!" << " " << pTarget->custom_data[oncoprotein_i] << std::endl;
            attach_cells( pAttacker, pTarget );
        }
        
        return true;
    }
    
    return false;
}

bool immune_cell_attempt_apoptosis( Cell* pAttacker, Cell* pTarget, double dt )
{
    static int oncoprotein_i = pTarget->custom_data.find_variable_index( "oncoprotein" );
    static int apoptosis_model_index = pTarget->phenotype.death.find_death_model_index( "apoptosis" );
    static int kill_rate_index = pAttacker->custom_data.find_variable_index( "kill_rate" );
    
    double oncoprotein_saturation =
        pAttacker->custom_data["oncoprotein_saturation"]; // 2.0;
    double oncoprotein_threshold =
        pAttacker->custom_data["oncoprotein_threshold"]; // 0.5; // 0.1;
    double oncoprotein_difference = oncoprotein_saturation - oncoprotein_threshold;

    // new
    if( pTarget->custom_data[oncoprotein_i] < oncoprotein_threshold )
    { return false; }
    
    // new
    double scale = pTarget->custom_data[oncoprotein_i];
    scale -= oncoprotein_threshold;
    scale /= oncoprotein_difference;
    if( scale > 1.0 )
    { scale = 1.0; }
    
    // if numerical conditions are met and tumor cell is not PDL1+
    if( (UniformRandom() < pAttacker->custom_data[kill_rate_index] * scale * dt) && (pTarget->custom_data["PDL1"] == 1)) // PDL1 value is either 0 if survived, or 1. So T cell can attach to PDL1+ cells but won't be able to kill them.
    {
//        std::cout << "\t\t kill!" << " " << pTarget->custom_data[oncoprotein_i] << std::endl;
        return true;
    }
    return false;
}

bool immune_cell_trigger_apoptosis( Cell* pAttacker, Cell* pTarget )
{
    static int apoptosis_model_index = pTarget->phenotype.death.find_death_model_index( "apoptosis" );
    
    // if the Target cell is already dead, don't bother!
    if( pTarget->phenotype.death.dead == true )
    { return false; }

    pTarget->start_death( apoptosis_model_index );
    return true;
}

void immune_cell_rule( Cell* pCell, Phenotype& phenotype, double dt )
{
    static int attach_lifetime_i = pCell->custom_data.find_variable_index( "attachment_lifetime" );
    
    if( phenotype.death.dead == true )
    {
        // the cell death functions don't automatically turn off custom functions,
        // since those are part of mechanics.
        
        // Let's just fully disable now.
        pCell->functions.custom_cell_rule = NULL;
        return;
    }
    
    // if I'm docked
    if( pCell->state.number_of_attached_cells() > 0 )
    {
        // attempt to kill my attached cell
        
        bool detach_me = false;
        
        if( immune_cell_attempt_apoptosis( pCell, pCell->state.attached_cells[0], dt ) )
        {
            immune_cell_trigger_apoptosis( pCell, pCell->state.attached_cells[0] );
            detach_me = true;
        }
        
        // decide whether to detach
        
        if( UniformRandom() < dt / ( pCell->custom_data[attach_lifetime_i] + 1e-15 ) )
        { detach_me = true; }
        
        // if I dettach, resume motile behavior
        
        if( detach_me )
        {
            detach_cells( pCell, pCell->state.attached_cells[0] );
            phenotype.motility.is_motile = true;
        }
        return;
    }
    
    // I'm not docked, look for cells nearby and try to docked
    
    // if this returns non-NULL, we're now attached to a cell
    if( immune_cell_check_neighbors_for_attachment( pCell , dt) )
    {
        // set motility off
        phenotype.motility.is_motile = false;
        return;
    }
    phenotype.motility.is_motile = true;
    
    return;
}

void adhesion_contact_function( Cell* pActingOn, Phenotype& pao, Cell* pAttachedTo, Phenotype& pat , double dt )
{
    std::vector<double> displacement = pAttachedTo->position - pActingOn->position;
    
    static double max_elastic_displacement = pao.geometry.radius * pao.mechanics.relative_detachment_distance;
    static double max_displacement_squared = max_elastic_displacement*max_elastic_displacement;
    
    // detach cells if too far apart
    
    if( norm_squared( displacement ) > max_displacement_squared )
    {
        detach_cells( pActingOn , pAttachedTo );
        return;
    }
    
    axpy( &(pActingOn->velocity) , pao.mechanics.attachment_elastic_constant , displacement );
    
    return;
}

/*
void tumor_cell_becomes_PDL1 (Cell* pCell, Phenotype& phenotype, double dt)
{
    // If T cell doesn’t kill tumor cell, tumor cell becomes PDL1+ w/ specified probability
    if ((pCell->state.number_of_attached_cells() > 0) && (pCell->phenotype.death.dead == false))
    {
        static int random_PDL1_probability = UniformRandom();
        if (random_PDL1_probability > 0.2)
            {
            //pCell -> custom_data["PDL1"] = 1.0;
            output[0] = "pink";
            output[1] = "pink";
            output[2] = "pink";
                return output;
            }
        else
        {
        return;
        }
    }
}
*/

// advice: first thing: check for neighbors. scan through all neighbors, and if there's a T cell then with some probability the T cell becomes activated and once in contact with an activated T cell, that's one switches to PDL1- or PDL1+.
// starting w/ PDL1- cell, if cell is immune-cell-attached,
/*
std::vector<std::string> cancer_cell_PDL1_coloring_function( Cell* pCell)
{
    std::vector< std::string > output = paint_by_number_cell_coloring(pCell);
    if (pCell->phenotype.death.dead==false) //((pCell->state.number_of_attached_cells() > 0) && (pCell->phenotype.death.dead == false)) // && (UniformRandom>0.2) // check
        // ADD in random number later
    {
        // convert cells that have survived immune attack to pink
        output[0]="pink";
        output[1]="pink";
        output[2]="pink";
        output[3]="pink";
        return output;
    }
    return output;
}
*/
int sum_dead_cells_over_time_window ()
{
    static int dead_cell_counter = 0;
    Cell* pCell = NULL;
    // loop over all cells
    for ( int i=0; i < (*all_cells).size(); i++ )
    {
        pCell = (*all_cells)[i];
        // if cell is dead increase counter
        if (pCell->phenotype.death.dead == true)
        {
        dead_cell_counter++;
        }
    }
    // currently only takes the number of dead in current step, not summed over time window
    // FIX
    //static int num_deaths_current_step = Cell_container->num_deaths_in_current_step;
    return dead_cell_counter;
    
}

// recruit number of T cells based on function provided by Gong et al Cess et al models
void recruit_T_cells ()
{
    Cell_Definition* pCell = find_cell_definition( "cancer cell" );

    // retrieve ka (mutational burden) and ki (neoantigen strength)
    static double ka = pCell->custom_data["mutational_burden"];
    static double ki = pCell->custom_data["neoantigen_strength"];
    static double r1 = pCell->custom_data["r1"];
    
    // calculate rate of tumor recruitment
    double T_cell_recruit_rate = ka*(double)sum_dead_cells_over_time_window()*r1/((1/ki)+(double)sum_dead_cells_over_time_window());

    double tumor_radius = -9e9; // 250.0;
    double temp_radius = 0.0;
    
    // for the loop, deal with the (faster) norm squared and ONLY tumor cells
    for( int i=0; i < (*all_cells).size() && (*all_cells)[i]->type == 0; i++ )
    {
        temp_radius = norm_squared( (*all_cells)[i]->position );
        if( temp_radius > tumor_radius )
        { tumor_radius = temp_radius; }
    }
    // now square root to get to radius
    tumor_radius = sqrt( tumor_radius );
    
    // if this goes wackadoodle, choose 250
    if( tumor_radius < 250.0 )
    { tumor_radius = 250.0; }
    
    std::cout << "current tumor radius: " << tumor_radius << std::endl;
    
    // now seed immune cells, rate (Gong et al) times diffusion time
    int number_of_immune_cells =
    T_cell_recruit_rate*mechanics_dt;
    
    // count number of immune cells
    std::cout << "current num T cells " << number_of_immune_cells << std::endl;
    
    double radius_inner = tumor_radius +
    parameters.doubles("initial_min_immune_distance_from_tumor"); 30.0; // 75 // 50;
    double radius_outer = radius_inner +
        parameters.doubles("thickness_of_immune_seeding_region"); // 75.0; // 100; // 1000 - 50.0;
    
    double mean_radius = 0.5*(radius_inner + radius_outer);
    double std_radius = 0.33*( radius_outer-radius_inner)/2.0;
    
    for( int i=0 ;i < number_of_immune_cells ; i++ )
    {
        double theta = UniformRandom() * 6.283185307179586476925286766559;
        double phi = acos( 2.0*UniformRandom() - 1.0 );
        
        double radius = NormalRandom( mean_radius, std_radius );
        
        Cell* pCell = create_cell( *pImmuneCell );
        pCell->assign_position( radius*cos(theta)*sin(phi), radius*sin(theta)*sin(phi), radius*cos(phi) );
    }
    return;
}


// macrophage functions
void macrophage_rule( Cell* pCell, Phenotype& phenotype, double dt )
{
    ///////////////////////////////
    /*
    static int IL4_ID = microenvironment.find_density_index( "IL4" );
    static int IFNg_ID = microenvironment.find_density_index( "IFNg" );
    
    // if macrophage is undifferentiated, determine whether they should become M1 or M2
    if (pCell -> custom_data["macrophage_identity"] == 0)
    {
        /*
        // if there's more IL4 than IFNg, become M2
        if (pCell->nearest_density_vector()[IL4_ID] > pCell->nearest_density_vector()[IFNg_ID])
        {
            pCell -> custom_data["macrophage_identity"] = 2;
        }
        
        // otherwise become M1
        else
        {
            pCell -> custom_data["macrophage_identity"] = 1;
        }
        
    }
    
    // if macrophage is M1
    if (pCell -> custom_data["macrophage_identity"] == 1)
    {
        // do nothing for now
        return;
    }
     
    
    // if macrophage is M2
    if (pCell -> custom_data["macrophage_identity"] == 2)
    {
        // secrete IL-4
        phenotype.secretion.secretion_rates[IL4_ID] = 10;
        // dampen immune-attack
        
    }
    */
    
    return;
}

void macrophage_motility( Cell* pCell, Phenotype& phenotype, double dt )
{
    // macrophages are motile and chemotaxis towards IL4 or IFNg gradient
    phenotype.motility.is_motile = true;
    
    /*
    // follow IL4 gradient, implement later
    phenotype.motility.migration_bias_direction = pCell->nearest_gradient(IL4_index);
    normalize( &( phenotype.motility.migration_bias_direction ) );
    */
    return;
}
