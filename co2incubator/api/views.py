from django.shortcuts import render
from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework.permissions import BasePermission, IsAuthenticated, SAFE_METHODS
from rest_framework import generics
from .serializer import estufa_data_serializer
from .models import estufa1_data


class ReadOnly(BasePermission):
    def has_permission(self, request, view):
        return request.method in SAFE_METHODS

# Create your views here.
class ChartData(generics.ListCreateAPIView):
    permission_classes = [IsAuthenticated|ReadOnly]

    queryset = estufa1_data.objects.all()
    serializer_class = estufa_data_serializer
"""
    def get(self, request, format=None):
        
    #    Return a list of all users.
    
        labels = ['Users', 'Blue', 'Yellow', 'Green', 'Purple', 'Orange']
        default_items = [3, 2, 2, 5, 0, 3, 2]
        data = {
            "labels":labels,
            "default":default_items,
        }
        return Response(data)"""