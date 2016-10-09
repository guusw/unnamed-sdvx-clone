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
	int WriteHandler(BinaryStream* stream, unsigned char *buffer, size_t size)
	{
		stream->Serialize(buffer, size);
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

	bool CollectEvents(Vector<yaml_event_t>& events, Node& node, NodeType hint = NodeType::Undefined)
	{
		yaml_event_t event = {};
		String tagString = node.GetTag();
		auto tag = tagString.empty() ? nullptr : (yaml_char_t*)*tagString;

		if(hint == NodeType::Mapping)
		{
			if(!yaml_mapping_start_event_initialize(&event, nullptr, tag, tag ? 0 : 1, YAML_ANY_MAPPING_STYLE))
				return false;
			events.Add(event);

			for(auto& inner : node.AsMapping())
			{
				if(!CollectEvents(events, *inner.key, NodeType::Scalar))
					return false;
				if(!CollectEvents(events, *inner.value, NodeType::Undefined))
					return false;
			}

			memset(&event, 0, sizeof(yaml_event_t));
			yaml_mapping_end_event_initialize(&event);
			events.Add(event);
		}
		else if(hint == NodeType::Scalar)
		{
			String scalarString = node.AsScalar().ToString();
			auto value = (yaml_char_t*)*scalarString;
			if(!yaml_scalar_event_initialize(&event, nullptr, tag, value, (int)scalarString.size(), 
				tag ? 0 : 1, tag ? 0 : 1, YAML_ANY_SCALAR_STYLE))
				return false;
			events.Add(event);
		}
		else if(hint == NodeType::Sequence)
		{
			if(!yaml_sequence_start_event_initialize(&event, nullptr, tag, tag ? 0 : 1, YAML_ANY_SEQUENCE_STYLE))
				return false;
			events.Add(event);

			for(auto inner : node.AsSequence())
			{
				if(!CollectEvents(events, *inner, NodeType::Undefined))
					return false;
			}

			memset(&event, 0, sizeof(yaml_event_t));
			yaml_sequence_end_event_initialize(&event);
			events.Add(event);
		}
		else if(hint == NodeType::Undefined)
		{
			if(node.IsMapping())
			{
				return CollectEvents(events, node, NodeType::Mapping);
			}
			else if(node.IsSequence())
			{
				return CollectEvents(events, node, NodeType::Sequence);
			}
			else if(node.IsScalar())
			{
				return CollectEvents(events, node, NodeType::Scalar);
			}
		}
		return true;
	}
	bool YamlStream::Write(class BinaryStream& stream, Yaml::Node& rootNode)
	{
		yaml_emitter_t emitter;
		Vector<yaml_event_t> events;
		bool success = true;

		yaml_emitter_initialize(&emitter);

		// Setup the emitter to write to the stream
		union
		{
			int(*myWriteHandler)(BinaryStream *data, unsigned char *buffer, size_t size);
			yaml_write_handler_t* writeHandler;
		};
		myWriteHandler = WriteHandler;

		yaml_emitter_set_output(&emitter, writeHandler, &stream);

		yaml_event_t event;

		// STREAM START
		yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
		if(!yaml_emitter_emit(&emitter, &event))
			goto _fail;
		yaml_event_delete(&event);

		// DOC START
		yaml_document_start_event_initialize(&event, nullptr, nullptr, nullptr, 1);
		if(!yaml_emitter_emit(&emitter, &event))
			goto _fail;
		yaml_event_delete(&event);

		// Collect events
		if(!CollectEvents(events, rootNode))
			goto _fail;

		for(auto& event : events)
		{
			yaml_emitter_emit(&emitter, &event);
		}

		// DOC END
		yaml_document_end_event_initialize(&event, 1);
		if(!yaml_emitter_emit(&emitter, &event))
			goto _fail;
		yaml_event_delete(&event);

		yaml_stream_end_event_initialize(&event);
		if(!yaml_emitter_emit(&emitter, &event))
			goto _fail;
		yaml_event_delete(&event);

		goto _end;

	_fail:
		success = false;
	_end:

		// Cleanup emitter
		yaml_emitter_delete(&emitter);
		return success;
	}
}