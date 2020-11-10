
import meshio
from meshio._mesh import CellBlock
import numpy as np
import argparse
import logging


def convert_abaqus_to_gmsh(input_mesh, output_mesh, logger=None):
    """
    @brief Convert an abaqus mesh to gmsh 2 format, preserving nodeset information
    @param input_mesh path of the input abaqus file
    @param output_mesh path of the output gmsh file
    @param logger an instance of logging.Logger
    """
    if not logger:
      logging.basicConfig(level=logging.WARNING)
      logger = logging.getLogger(__name__)

    # Load the mesh
    logger.info('Reading abaqus mesh...')
    mesh = meshio.Mesh.read(input_mesh, "abaqus")

    # Convert the element regions to tags
    logger.info('Converting region tags...')
    region_list = list(mesh.cell_sets.keys())
    n_regions = len(region_list)
    cell_ids = np.zeros(len(mesh.cells[0][1]), dtype=int) - 1
    for ii, region in enumerate(region_list):
        mesh.field_data[region] = [ii + 1, 3]
        cell_ids[mesh.cell_sets[region][0]] = ii + 1

    # Add to the meshio datastructure
    mesh.cell_data['gmsh:physical'] = [cell_ids]
    mesh.cell_data['gmsh:geometrical'] = [cell_ids]
    if (-1 in cell_ids):
        logger.warning('Some element regions did not convert correctly to tags!')

    # Build the face elements
    logger.info('Converting nodesets to face elements, tags...')
    new_tris, tri_nodeset, tri_region = [], [], []
    new_quads, quad_nodeset, quad_region = [], [], []

    for nodeset_id, nodeset_name in enumerate(mesh.point_sets):
        logger.info('  %s' % (nodeset_name))
        mesh.field_data[nodeset_name] = [nodeset_id + n_regions + 1, 2]
        nodeset = mesh.point_sets[nodeset_name]

        # Search the elements
        for element_id in range(len(mesh.cells[0][1])):
            # Find any matching nodes
            matching_nodes = []
            for node_id in mesh.cells[0][1][element_id]:
                if node_id in nodeset:
                    matching_nodes.append(node_id)

            # Find the region
            region_id = -1
            if (len(matching_nodes) >= 3):
                for region in region_list:
                    if (element_id in mesh.cell_sets[region][0]):
                        region_id = mesh.field_data[region][0]

            # Test to see if they match a quad or triangle
            tag_id = mesh.field_data[nodeset_name][0]
            n_matching = len(matching_nodes)
            if (n_matching == 3):
                new_tris.append(matching_nodes)
                tri_nodeset.append(tag_id)
                tri_region.append(region_id)

            elif (n_matching == 4):
                new_quads.append(matching_nodes)
                quad_nodeset.append(tag_id)
                quad_region.append(region_id)

            elif (n_matching > 4):
                logger.warning('  Discarding an element with an unexpected number of nodes')
                logger.warning('    n_nodes=%i, element=%i, set=%s' % (n_matching, element_id, nodeset_name))

    # Add new tris
    if new_tris:
        logger.info('  Adding %i new triangles...' % (len(new_tris)))
        if (-1 in tri_region):
            logger.warning('Traingles with empty region information found!')
        mesh.cells.append(CellBlock('triangle', np.array(new_tris)))
        mesh.cell_data['gmsh:geometrical'].append(np.array(tri_region))
        mesh.cell_data['gmsh:physical'].append(np.array(tri_nodeset))

    # Add  new quads
    if new_quads:
        logger.info('  Adding %i new quads...' % (len(new_quads)))
        if (-1 in quad_region):
            logger.warning('Quads with empty region information found!')
        mesh.cells.append(CellBlock('quad', np.array(new_quads)))
        mesh.cell_data['gmsh:geometrical'].append(np.array(quad_region))
        mesh.cell_data['gmsh:physical'].append(np.array(quad_nodeset))

    # Write the final mesh
    logger.info('Writing gmsh mesh...')
    meshio.write(output_mesh, mesh, file_format="gmsh22", binary=False)

    logger.info('Done!')


def main():
    """
    @brief Entry point for the abaqus convertor console script
    @arg input_mesh Input abaqus file name
    @arg output_mesh Output gmsh file name
    """

    # Parse the user arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('input', type=str, help='Input abaqus mesh file name')
    parser.add_argument('output', type=str, help='Output gmsh mesh file name')
    parser.add_argument('-v', '--verbose', help='Increase verbosity level', action="store_true")
    args = parser.parse_args()

    # Set up a logger
    logging.basicConfig(level=logging.WARNING)
    logger = logging.getLogger(__name__)
    if args.verbose:
      logger.setLevel(logging.INFO)

    convert_abaqus_to_gmsh(args.input, args.output, logger)


