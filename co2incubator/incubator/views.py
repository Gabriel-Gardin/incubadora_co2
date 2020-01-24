from django.shortcuts import render, HttpResponse
from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework import authentication, permissions
from django.contrib.auth.models import User
import json


def incubator(request):
    return render(request, 'incubator/charts.html')

def home(request):
    return  render(request, "incubator/home.html")


def contact(request):
    return render(request, "incubator/contact.html")

