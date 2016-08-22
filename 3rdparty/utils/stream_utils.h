#ifndef __STREAM_UTILS_H__
#define __STREAM_UTILS_H__

namespace stream_utils {

template< typename ARRT >
size_t write_array(stream* pstream, const ARRT& arr)
{
    typename ARRT::const_iterator it = arr.begin();
    typename ARRT::const_iterator end = arr.end();

    pstream->write_dword(arr.size());
    for(;it!=end;++it)
    {
        const typename ARRT::value_type* pp = &*it;
        pp->save(pstream);
    }
}

template< typename ARRT >
size_t write_array(stream* pstream, const ARRT& arr, size_t (*saver_fp)(const typename ARRT::value_type*, stream*))
{
    typename ARRT::const_iterator it = arr.begin();
    typename ARRT::const_iterator end = arr.end();

    size_t counter = 0;
    pstream->write_dword(arr.size());
    for(;it!=end;++it)
    {
        const typename ARRT::value_type* pp = &*it;
        counter += saver_fp(pp, pstream);
    }

    return counter;
}

template< typename ARRT >
size_t read_array(const stream* pstream, ARRT& arr, size_t (*loader_fp)(const stream*, ARRT&))
{
    size_t counter = 0;
    size_t size = pstream->read_dword();
    for(size_t ui=0;ui<size;++ui)
    {
        counter += loader_fp(pstream, arr);
    }
    return counter;
}


size_t read_stdstring(const stream* pstream, std::string& str);
size_t write_stdstring(stream* pstream, const std::string& str);

const char* load_file(const char* fname);

};
#endif // __STREAM_UTILS_H__
