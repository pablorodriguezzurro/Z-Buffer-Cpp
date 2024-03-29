#include "Scene.hpp"
#include "Model.hpp"
#include "Camera.hpp"

#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace rapidxml;
using namespace std;

namespace przurro
{
	Scene::Scene(String & inputAssetsFolderPath, size_t inputWidth, size_t inputHeight, float nearPlaneD, float farPlaneD, float fovDegrees)
		: assetsFolderPath(inputAssetsFolderPath),
		activeCamera(new Camera(nearPlaneD, farPlaneD, fovDegrees, inputWidth, inputHeight)),
		colorBuffer(inputWidth, inputHeight),
		rasterizer(colorBuffer)
	{
		models = {};
		load_scene(assetsFolderPath);
	}

	void Scene::update()
	{
		for(auto & model : models)
		{
			model.second->update_transform();
			model.second->update(activeCamera.get(),rasterizer, &infiniteLight);
		}
	}

	void Scene::draw()
	{
		//The rasterizer is cleared
		rasterizer.clear();

		for (auto & model : models)
		{
			model.second->draw(rasterizer);
		}

		// Se copia el frameb�ffer oculto en el frameb�ffer de la ventana:

		rasterizer.get_color_buffer().gl_draw_pixels(0, 0);
	}

	bool Scene::load_scene(String & xmlFilePath)
	{
		bool loaded = false;

		String xmlFilePathTemp = xmlFilePath; //Apparently, rapixml need to modify the string

		// Read the xml file into a vector
		ifstream theFile(xmlFilePathTemp);
		vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
		buffer.push_back('\0');

		rapidxml::xml_document<> doc;
		XML_Node * rootNode;

		// parse the buffer using the xml file parsing library into doc 
		doc.parse<0>(&buffer[0]);
		//find our root node
		rootNode = doc.first_node("scene");

		if (!rootNode || rootNode->type() != node_element)
			return false;

		if (String(rootNode->name()) != "scene")
			return false;

		return load_models(rootNode->first_node("models")) && load_cameras(rootNode->first_node("cameras"));
	}

	bool Scene::load_models(XML_Node * modelNodeRoot)
	{
		if (!modelNodeRoot || modelNodeRoot->type() != node_element)
			return false;

		if (String(modelNodeRoot->name()) != "models")
			return false;

		XML_Node * model = modelNodeRoot->first_node("model");

		if (!model || model->type() != node_element)
		{
			return false;
		}

		for (model; model; model = model->next_sibling())
		{
			if (model->type() == node_element)
			{
				// Iterate over each entity node
				if (String(model->name()) == "model")
				{
					XML_Attr * attribPath = model->first_attribute("path"), * attribName = model->first_attribute("name");;
					
					if (!attribPath || !attribName)
						return false;

					if(String(attribPath->name()) != "path" || String(attribName->name()) != "name")
						return false;

					models[attribName->value()] = Model_sptr(new Model(attribPath->value(), attribName->value()));

					//models[attribName->value()]->set_parent(activeCamera->get_transform());

					XML_Node * modelProperty = model->first_node();

					for (modelProperty; modelProperty; modelProperty = modelProperty->next_sibling())
					{
						if (!modelProperty || modelProperty->type() != node_element)
							break;

						if(!load_property(modelProperty, models[attribName->value()].get()))
							return false;
					}
				}
			}
			else break;	
		}

		return true;
	}
	bool Scene::load_cameras(XML_Node * cameraNode)
	{
		if (!cameraNode || cameraNode->type() != node_element)
			return false;

		if (String(cameraNode->name()) != "cameras")
			return false;

		XML_Node * camera = cameraNode->first_node("camera");

		if (!camera || camera->type() != node_element)
		{
			return false;
		}

		if (String(camera->name()) != "camera")
			return false;

		XML_Node * cameraProperty = camera->first_node();

		for (cameraProperty;cameraProperty; cameraProperty = cameraProperty->next_sibling())
		{
			if (!cameraProperty || cameraProperty->type() != node_element)
				break;

			if(!load_property(cameraProperty, nullptr, activeCamera.get()))
				return false;
		}

		return true;
	}
	bool Scene::load_property(XML_Node * attribute, Model * model, Camera * camera)
	{
		String name = attribute->name();
		String value = attribute->value();

		if (name == "target")
		{
			if (!model_exists(value))
			{
				return false;
			}
			if (camera)
			{
				activeCamera->set_target(&models[value]->get_reference_to_position());
			}
		}
		else if (name == "parent")
		{
			if (camera)
			{
				if (!model_exists(value))
					return false;
				
				activeCamera->set_target(&models[value]->get_reference_to_position());
			}
			if (model)
			{
				if (!model_exists(value))
					return false;
				model->set_parent(models[value]->get_transform());
			}
		}
		else if (name == "position" || name == "rotation" || name == "color" || name == "constant_rotation"|| name == "default_color" || name == "mesh_color")
		{
			vector<String> stringChunks  = String_Utilities::string_splitter(value, ',');
			
			if (stringChunks.size() == 3)
			{
				Vector3f values({stof(stringChunks[0]), stof(stringChunks[1]), stof(stringChunks[2])});

				if (name == "position")
				{
					if(model)
						model->set_position(values);
					else if (camera)
						camera->set_position(values);
				}
				if (model)
				{
					if (name == "rotation")
					{
						model->set_rotation(values);
					}
					else if (name == "constant_rotation")
					{
						model->set_constant_rotation(values);
					}
				}
			}
			else if (stringChunks.size() == 4)
			{
				if (model)
				{
					Vector4i values({ stoi(stringChunks[X]),stoi(stringChunks[Y]),stoi(stringChunks[Z]), stoi(stringChunks[W]) });

					if (name == "default_color")
					{
						model->set_default_color(values);
						return true;
					}
					if (name == "mesh_color")
					{
						XML_Attr * attribName = attribute->first_attribute("mesh_name");

						if (!attribName)
							return false;
						if ((String)attribName->name() != "mesh_name")
							return false;

						String meshName = attribName->value();

						return model->set_mesh_color(meshName, values);
					}
				}
			}
		}
		else if (name == "scale") 
		{
			model->set_scale(stof(value));
		}
		else return false;

		return true;
	}
	bool Scene::model_exists(const String & name)
	{
		auto iterator = models.find(name);

		if (iterator == models.end())
			return false;

		return true;
	}
}