#include "stdafx.h"
#include "Yaml.hpp"
#include "YamlStream.hpp"
#include "Shared/BinaryStream.hpp"
#include "Shared/TextStream.hpp"

#include "yaml.h"

namespace Yaml
{

	int ReadHandler(BinaryStream* stream, unsigned char *buffer, size_t size, size_t *size_read)
	{
		*size_read = stream->Serialize(buffer, size);
		return 1;
	}

	// Build structures
	enum class NodeType
	{
		Undefined,
		Mapping,
		Sequence,
		Scalar,
		Pair
	};
	struct StackNode
	{
		union
		{
			Node* node;
			Mapping* mapping;
			Sequence* sequence;
			Scalar* scalar;
			Node* pair[2];
		};
		NodeType type = NodeType::Undefined;
	};

	class Parser
	{
	public:
		bool Parse(Node*& data, class BinaryStream& stream)
		{
			yaml_parser_t parser;
			yaml_parser_initialize(&parser);

			// Setup the reader to read from the stream
			union
			{
				int(*myReadHandler)(BinaryStream *data, unsigned char *buffer, size_t size, size_t *size_read);
				yaml_read_handler_t* readHandler;
			};
			myReadHandler = ReadHandler;
			yaml_parser_set_input(&parser, readHandler, &stream);

			bool done = false;
			bool error = false;
			while(!done)
			{
				yaml_event_t event;
				yaml_parser_parse(&parser, &event);

				if(parser.error != YAML_NO_ERROR)
				{
					Logf("YAML Parser error [%s] at line %d, col %d", Logger::Error, parser.problem,
						parser.problem_mark.line + 1, parser.problem_mark.column);
					error = true;
					break;
				}

				if(event.type == YAML_STREAM_START_EVENT)
				{
					// Begin stream?
				}
				else if(event.type == YAML_MAPPING_START_EVENT)
				{
					StackNode& node = m_stack.AddBack();
					node.mapping = new Mapping();
					node.type = NodeType::Mapping;

					if(event.data.mapping_start.tag)
						node.node->SetTag((char*)event.data.mapping_start.tag);
					if(event.data.mapping_start.anchor)
						m_AddAnchor(event.data.mapping_start.anchor, node.node);
				}
				else if(event.type == YAML_SEQUENCE_START_EVENT)
				{
					StackNode& node = m_stack.AddBack();
					node.sequence = new Sequence();
					node.type = NodeType::Sequence;

					if(event.data.sequence_start.tag)
						node.node->SetTag((char*)event.data.sequence_start.tag);
					if(event.data.sequence_start.anchor)
						m_AddAnchor(event.data.sequence_start.anchor, node.node);
				}
				else if(event.type == YAML_MAPPING_END_EVENT)
				{
					m_FinishNode();
				}
				else if(event.type == YAML_SEQUENCE_END_EVENT)
				{
					m_FinishNode();
				}
				else if(event.type == YAML_SCALAR_EVENT)
				{
					String str = String(event.data.scalar.value, event.data.scalar.value + event.data.scalar.length);

					StackNode& node = m_stack.AddBack();
					node.scalar = new Scalar(str);
					node.type = NodeType::Scalar;

					if(event.data.scalar.tag)
						node.node->SetTag((char*)event.data.scalar.tag);
					if(event.data.scalar.anchor)
						m_AddAnchor(event.data.scalar.anchor, node.node);

					m_FinishNode();
				}
				else if(event.type == YAML_ALIAS_EVENT)
				{
					Node** foundReference = m_anchors.Find((char*)event.data.alias.anchor);
					assert(foundReference);
					Node& reference = **foundReference;

					StackNode& node = m_stack.AddBack();
					node.node = &reference;

					if(reference.IsMapping())
					{
						node.type = NodeType::Mapping;
					}
					else if(reference.IsSequence())
					{
						node.type = NodeType::Sequence;
					}
					else
					{
						node.type = NodeType::Scalar;
					}

					m_FinishNode();
				}
				else if(event.type == YAML_STREAM_END_EVENT)
					done = true;

				yaml_event_delete(&event);
			}

			yaml_parser_delete(&parser);

			if(!m_stack.empty())
			{
				for(auto n : m_stack)
				{
					if(n.type == NodeType::Pair)
					{
						if(n.pair[0])
							delete n.pair[0];
						if(n.pair[1])
							delete n.pair[1];
					}
					delete n.node;
				}
				m_stack.clear();
			}

			data = m_rootNode;
			return m_rootNode != nullptr;
		}

	private:		
		// Creates an anchor binding
		void m_AddAnchor(yaml_char_t* anchor, Node* node)
		{
			m_anchors.Add((char*)anchor, node);
		}

		// Removes a node from the stack and adds it to it's parent
		bool m_FinishNode()
		{
			assert(!m_stack.empty());
			StackNode node = m_stack.back();
			m_stack.pop_back();
			if(m_stack.empty())
				m_rootNode = node.mapping;
			else
			{
				StackNode& newParent = m_stack.back();
				if(newParent.type == NodeType::Sequence)
				{
					// Insert into sequence
					newParent.sequence->Add((Node*)node.node);	
				}
				else if(newParent.type == NodeType::Mapping)
				{
					if(node.type != NodeType::Pair)
					{
						// Create a new pair
						StackNode& pair = m_stack.AddBack();
						pair.type = NodeType::Pair;
						pair.pair[0] = node.node;
					}
					else
					{
						// Insert into object
						assert(node.pair[0]->IsScalar());
						Scalar& scalarKey = node.pair[0]->AsScalar();
						newParent.mapping->Add(new Scalar(scalarKey), (Node*)node.pair[1]);
						delete node.pair[0];
					}
				}
				else if(newParent.type == NodeType::Pair)
				{
					newParent.pair[1] = node.node;
					m_FinishNode();
				}
				else
				{
					// Invalid mapping
					assert(false);
				}
			}
			return true;
		}

		Map<String, Node*> m_anchors;
		List<StackNode> m_stack;
		Node* m_rootNode = nullptr;
	};

	Ref<Node> YamlStream::Read(class BinaryStream& stream)
	{
		Node* ret;
		Parser parser;
		if(!parser.Parse(ret, stream))
			return Ref<Node>();
		return Ref<Node>(ret);
	}
}