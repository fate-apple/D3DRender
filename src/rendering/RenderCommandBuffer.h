#pragma once
#include <algorithm>
#include <vector>

#include "pch.h"

#include "Material.h"
#include "core/Memory.h"

//sort keys instead values to reduce copy/move heavy struct
template <typename TKey, typename TValue>
struct SortKeyVector
{
private:
    struct SortKey
    {
        TKey key;
        uint32 index;
    };
    std::vector<SortKey> keys;
    std::vector<TValue> values;
public:
    void Sort()
    {
        std::sort(keys.begin(), keys.end(), [](const SortKey& a, const SortKey& b) { return a.key < b.key; });
    }
    void Clear()
    {
        keys.clear();
        values.clear();
    }

    uint64 Size() const {return keys.size();}
    
    void PushBack(TKey key, const TValue& value)
    {
        uint32 index = static_cast<uint32>(Size());
        keys.push_back({key,index});
        values.push_back(value);
    }

    void PushBack(TKey key, TValue&& value)
    {
        uint32 index = static_cast<uint32>(Size());
        keys.push_back({key,index});
        values.push_back(std::move(value));
    }

    template<typename... TArgs>
    TValue& EmplaceBack(TKey key, TArgs&&... args)
    {
        uint32 index = static_cast<uint32>(Size());
        keys.push_back({key,index});
        return values.emplace(std::forward<TArgs>(args)...);
    }

    struct iterator
    {
        typename std::vector<SortKey>::iterator keyIterator;
        TValue* values;

        friend  bool operator ==(const iterator& a, const iterator& b)
        {
            return  a.keyIterator == b.keyIterator && a.values ==b.values;
        }
        friend  bool operator !=(const iterator& a, const iterator& b)
        {
            return !(a==b);
        }
        iterator& operator++()
        {
            ++keyIterator;
            return *this;
        }
        TValue& operator*()
        {
            return values[keyIterator->index];
        }
    };
    struct const_iterator
    {
        typename std::vector<SortKey>::const_iterator keyIterator;
        const TValue* values;
        
        friend  bool operator ==(const const_iterator& a, const const_iterator& b)
        {
            return  a.keyIterator == b.keyIterator && a.values ==b.values;
        }
        friend  bool operator !=(const const_iterator& a, const const_iterator& b)
        {
            return !(a==b);
        }
        const_iterator& operator++()
        {
            ++keyIterator;
            return *this;
        }
        const TValue& operator*()
        {
            return values[keyIterator->index];
        }
    };
    iterator begin()        //keep same with stl to use for-each
    {
        return iterator{keys.begin(), values.data()};
    }
    iterator end()
    {
        return iterator{keys.end(), values.data()};
    }
    const_iterator begin() const
    {
        return const_iterator{keys.begin(), values.data()};
    }
    const_iterator end() const
    {
        return const_iterator{keys.end(), values.data()};
    }
};

typedef void (*PipelineSetupFunc)(DxCommandList*, const CommonMaterialInfo&);
typedef void (*PipelineRenderFunc)(DxCommandList*, const mat4& viewProj, void* command);

template <typename Tkey>
struct RenderCommandBuffer
{
private:
    struct CommandKey
    {
        Tkey key;
        void* data;
    };
    struct CommandHeader
    {
        PipelineSetupFunc setup;
        PipelineRenderFunc render;
    };
    struct CommandWrapperBase
    {
        CommandHeader header;
        virtual ~CommandWrapperBase(){}
    };
    std::vector<CommandKey> keys;
    MemoryArena arena;

    /**
     * \brief  allocate memory, create lambda which call pass in setup/render function, and put the wrapper in keys;
     * \tparam TPipeline 
     * \tparam TCommand 
     * \param sortKey 
     * \return 
     */
    template<typename TPipeline, typename TCommand>
    TCommand& PushInternal(Tkey sortKey)
    {
        struct CommandWrapper : CommandWrapperBase
        {
            TCommand command;
        };
        CommandWrapper* wrapper = arena.Allocate<CommandWrapper>();
        new(wrapper) CommandWrapper;
        wrapper->header.setup = TPipeline::setup;
        // call TPipeline render
        wrapper->header.render = [](DxCommandList* cl, const mat4& viewProj, void* data)
        {
            CommandWrapper* cw = static_cast<CommandWrapper*>(data);
            TPipeline::render(cl, viewProj, cw->command);
        };
        CommandKey key;
        key.key = sortKey;
        key.data = wrapper;
        keys.push_back(key);
        return wrapper->command;
    }
    
public:
    RenderCommandBuffer()
    {
        arena.Initialize(0, GB(4));
        keys.reserve(128);
    }

    uint64 Size() const { return keys.size();}
    void Sort() {std::sort(keys.begin(), keys.end(), [](const CommandKey& a, const CommandKey& b){return a.key < b.key;});}
    void Clear()
    {
        for(auto& key : keys) {
            CommandWrapperBase* wrapperBase = static_cast<CommandWrapperBase*>(key.data);
            wrapperBase->~command_wrapper_base();
        }

        arena.Reset();
        keys.clear();
    }
    // 
    template <typename TPipeline, typename TCommand, typename... Targs>
    TCommand& EmplaceBack(Tkey sortkey, Targs&& ...args)
    {
        TCommand& command = PushInternal<TPipeline, TCommand>(sortkey);
        new(&command) TCommand(std::forward<Targs>(args)...);
        return command;
    }
};